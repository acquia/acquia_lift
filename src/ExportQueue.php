<?php

namespace Drupal\acquia_perz;

use Drupal\Core\DependencyInjection\DependencySerializationTrait;
use Drupal\Core\Messenger\MessengerInterface;
use Drupal\Core\Queue\QueueFactory;
use Drupal\Core\Queue\QueueWorkerManager;
use Drupal\Core\Queue\SuspendQueueException;
use Drupal\Core\StringTranslation\StringTranslationTrait;
use Drupal\Core\Config\ImmutableConfig;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Render\RendererInterface;
use GuzzleHttp\Exception\TransferException;
use Drupal\acquia_perz\ExportTracker;

/**
 * Implements an Export Queue for CIS.
 */
class ExportQueue {

  use StringTranslationTrait;
  use DependencySerializationTrait;

  const DELETED = 'deleted';

  const EXPORTED = 'exported';

  const FAILED = 'failed';

  /**
   * The export tracker service.
   *
   * @var \Drupal\acquia_perz\ExportTracker
   */
  private $exportTracker;

  /**
   * The acquia perz entity settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_perz\Form\ContentPublishingForm
   */
  protected $entitySettings;

  /**
   * Renderer.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The Export Content Queue.
   *
   * @var \Drupal\Core\Queue\QueueInterface
   */
  protected $queue;

  /**
   * The Queue Worker.
   *
   * @var \Drupal\Core\Queue\QueueWorkerManager
   */
  protected $queueManager;

  /**
   * The messenger object.
   *
   * @var \Drupal\Core\Messenger\MessengerInterface
   */
  protected $messenger;

  /**
   * {@inheritdoc}
   */
  public function __construct(ExportTracker $export_tracker,
                              ImmutableConfig $entity_settings,
                              RendererInterface $renderer,
                              EntityTypeManagerInterface $entity_type_manager,
                              QueueFactory $queue_factory,
                              QueueWorkerManager $queue_manager,
                              MessengerInterface $messenger) {
    $this->exportTracker = $export_tracker;
    $this->entitySettings = $entity_settings;
    $this->renderer = $renderer;
    $this->entityTypeManager = $entity_type_manager;
    $this->queue = $queue_factory->get('acquia_perz_publish_export');
    $this->queueManager = $queue_manager;
    $this->messenger = $messenger;
  }

  /**
   * Obtains the number of items in the export queue.
   *
   * @return mixed
   *   The number of items in the export queue.
   */
  public function getQueueCount() {
    return $this->queue->numberOfItems();
  }

  /**
   * Add entity to the Export Queue.
   * @param string $action
   *  The action, possible values:
   *  - 'insert_or_update'
   *  - 'delete_entity'
   *  - 'delete_translation'
   * @param string $entity_type
   *  Entity type of the entity that should be exported.
   * @param integer $entity_id
   *  Id of the entity that should be exported.
   * @param string $langcode
   *  Language code of the entity translation that should be exported.
   * 'all' value means that all entity translations should be exported.
   */
  public function addQueueItem($action, $entity_type, $entity_id, $entity_uuid = '', $langcode = 'all') {
    if (empty($entity_uuid)) {
      $entity = $this->entityTypeManager
        ->getStorage($entity_type)
        ->load($entity_id);
      $entity_uuid = $entity->uuid();
    }
    $this->queue->createItem([
      'action' => $action,
      'entityType' => $entity_type,
      'entityId' => $entity_id,
      'uuid' => $entity_uuid,
      'langcode' => $langcode,
    ]);
  }

  /**
   * Remove all the export queue items.
   */
  public function purgeQueues() {
    $this->queue->deleteQueue();
  }

  /**
   * Rescan content and add it to the queue.
   */
  public function rescanContent() {
    $batch = [
      'title' => $this->t("Rescan Content Process"),
      'operations' => [],
      'finished' => [[$this, 'rescanBatchFinished'], []],
    ];
    $entity_types = $this->entitySettings->get('view_modes');
    foreach ($entity_types as $entity_type => $bundles) {
      // Check only bundles with at least one view mode activated
      // besides 'acquia_perz_preview_image' view mode.
      $available_bundles = [];
      foreach ($bundles as $bundle => $view_modes) {
        $view_modes = array_keys($view_modes);
        if (count($view_modes) === 1
          && in_array('acquia_perz_preview_image', $view_modes)) {
          continue;
        }
        $available_bundles[] = $bundle;
      }
      // Skip entity type without activated bundles.
      if (empty($available_bundles)) {
        continue;
      }
      $bundle_property_name = $this
        ->entityTypeManager
        ->getStorage($entity_type)
        ->getEntityType()
        ->getKey('bundle');
      $query = $this
        ->entityTypeManager
        ->getStorage($entity_type)
        ->getQuery();
      // Single-bundle entity types like 'user' don't use
      // bundle related property.
      if (!empty($bundle_property_name)) {
        $query = $query->condition($bundle_property_name, $available_bundles, 'IN');
      }
      $entity_ids = $query->execute();
      foreach ($entity_ids as $entity_id) {
        $batch['operations'][] = [[$this, 'rescanBatchProcess'], [$entity_type, $entity_id]];
      }
    }
    // Adds the batch sets.
    batch_set($batch);
  }

  /**
   * Rescan content batch processing callback.
   *
   * @param string $entity_type
   *  The entity type.
   * @param string $entity_id
   *  The entity id.
   * @param mixed $context
   *  The context array.
   */
  public function rescanBatchProcess($entity_type, $entity_id, &$context) {
    $this->addQueueItem('insert_or_update', $entity_type, $entity_id);
  }

  /**
   * Rescan content batch finished callback.
   *
   * @param bool $success
   *   Whether the batch process succeeded or not.
   * @param array $results
   *   The results array.
   * @param array $operations
   *   An array of operations.
   */
  public function rescanBatchFinished($success, array $results, array $operations) {
    if ($success) {
      $this->messenger->addMessage(t("The contents are successfully rescanned."));
    }
    else {
      $error_operation = reset($operations);
      $this->messenger->addMessage(t('An error occurred while processing @operation with arguments : @args', [
          '@operation' => $error_operation[0],
          '@args' => print_r($error_operation[0], TRUE),
        ]
      ));
    }
    // Providing a report on the items processed by the queue.
    $elements = [
      '#theme' => 'item_list',
      '#type' => 'ul',
      '#items' => $results,
    ];
    $queue_report = $this->renderer->render($elements);
    $this->messenger->addMessage($queue_report);
  }

  /**
   * Process all queue items with batch API.
   */
  public function exportQueueItems() {
    // Create batch which collects all the specified queue items and process
    // them one after another.
    $batch = [
      'title' => $this->t("Process Export Queue"),
      'operations' => [],
      'finished' => [[$this, 'exportBatchFinished'], []],
    ];

    // Count number of the items in this queue, create enough batch operations.
    for ($i = 0; $i < $this->getQueueCount(); $i++) {
      // Create batch operations.
      $batch['operations'][] = [[$this, 'exportBatchProcess'], []];
    }

    // Adds the batch sets.
    batch_set($batch);
  }

  /**
   * Common batch processing callback for all operations.
   *
   * @param mixed $context
   *   The context array.
   */
  public function exportBatchProcess(&$context) {
    $queueWorker = $this->queueManager->createInstance('acquia_perz_publish_export');

    // Get a queued item.
    if ($item = $this->queue->claimItem()) {
      try {
        // Generating a list of entities.
        $msg_label = $this->t('(@entity_type, @entity_id)', [
          '@entity_type' => $item->data['entityType'],
          '@entity_id' => $item->data['entityId'],
        ]);

        // Process item.
        $entities_processed = $queueWorker->processItem($item->data);
        if ($entities_processed == FALSE) {
          // Indicate that the item could not be processed.
          if ($entities_processed === FALSE) {
            $message = $this->t('There was an error processing entities: @entities and their dependencies. The item has been sent back to the queue to be processed again later. Check your logs for more info.', [
              '@entities' => $msg_label,
            ]);
          }
          else {
            $message = $this->t('No processing was done for entities: @entities and their dependencies. The item has been sent back to the queue to be processed again later. Check your logs for more info.', [
              '@entities' => $msg_label,
            ]);
          }
          $context['message'] = $message->jsonSerialize();
          $context['results'][] = $message->jsonSerialize();
        }
        else {
          // If everything was correct, delete processed item from the queue.
          $this->queue->deleteItem($item);

          // Creating a text message to present to the user.
          $message = $this->t('Processed entities: @entities and their dependencies (@count @label sent).', [
            '@entities' => $msg_label,
            '@count' => $entities_processed,
            '@label' => $entities_processed == 1 ? $this->t('entity') : $this->t('entities'),
          ]);
          $context['message'] = $message->jsonSerialize();
          $context['results'][] = $message->jsonSerialize();
        }

      }
      catch (\RuntimeException $e) {
        if ($e instanceof SuspendQueueException
          || $e instanceof TransferException) {
          switch ($item->data['action']) {
            case 'insert_or_update':
              $this->exportTracker->exportTimeout(
                $item->data['entityType'],
                $item->data['entityId'],
                $item->data['uuid'],
                $item->data['langcode']
              );
              break;

            case 'entity_delete':
            case 'translation_delete':
              $this->exportTracker->deleteTimeout(
                $item->data['entityType'],
                $item->data['entityId'],
                $item->data['uuid'],
                $item->data['langcode']
              );
              break;
          }
          $this->addQueueItem(
            $item->data['action'],
            $item->data['entityType'],
            $item->data['entityId'],
            $item->data['uuid'],
            $item->data['langcode']
          );
          $this->queue->deleteItem($item);
        }
      }
    }
  }

  /**
   * Batch finished callback.
   *
   * @param bool $success
   *   Whether the batch process succeeded or not.
   * @param array $results
   *   The results array.
   * @param array $operations
   *   An array of operations.
   */
  public function exportBatchFinished($success, array $results, array $operations) {
    if ($success) {
      $this->messenger->addMessage(t("The contents are successfully exported."));
    }
    else {
      $error_operation = reset($operations);
      $this->messenger->addMessage(t('An error occurred while processing @operation with arguments : @args', [
        '@operation' => $error_operation[0],
        '@args' => print_r($error_operation[0], TRUE),
      ]
      ));
    }

    // Providing a report on the items processed by the queue.
    $elements = [
      '#theme' => 'item_list',
      '#type' => 'ul',
      '#items' => $results,
    ];
    $queue_report = $this->renderer->render($elements);
    $this->messenger->addMessage($queue_report);
  }

}
