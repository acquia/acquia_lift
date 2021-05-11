<?php

namespace Drupal\acquia_perz\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Queue\QueueFactory;
use Drupal\Core\CronInterface;
use Drupal\Core\Database\Connection;
use Drupal\acquia_perz\ExportQueue;
use Drupal\Core\Entity\EntityTypeManagerInterface;

/**
 * Defines the form to export content via Queue.
 */
class ExportForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The Export Queue Service.
   *
   * @var \Drupal\acquia_perz\ExportQueue
   */
  protected $exportQueue;

  /**
   * The queue object.
   *
   * @var \Drupal\Core\Queue\QueueFactory
   */
  protected $queueFactory;

  /**
   * The database object.
   *
   * @var \Drupal\Core\Database\Connection
   */
  protected $database;

  /**
   * The CronInterface object.
   *
   * @var \Drupal\Core\CronInterface
   */
  protected $cron;

  /**
   * Constructor.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   Entity type manager service.
   * @param \Drupal\acquia_perz\ExportQueue $export_queue
   *   Export Queue service.
   * @param \Drupal\Core\Queue\QueueFactory $queue_factory
   *   Queue factory service to get new/existing queues for use.
   * @param \Drupal\Core\Database\Connection $database
   *   The database connection to be used.
   * @param Drupal\Core\CronInterface $cron
   *   The cron service.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager, ExportQueue $export_queue, QueueFactory $queue_factory, Connection $database, CronInterface $cron) {
    $this->entityTypeManager = $entity_type_manager;
    $this->exportQueue = $export_queue;
    $this->queueFactory = $queue_factory;
    $this->database = $database;
    $this->cron = $cron;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    $form = new static(
      $container->get('entity_type.manager'),
      $container->get('acquia_perz.export_queue'),
      $container->get('queue'),
      $container->get('database'),
      $container->get('cron')
    );
    $form->setMessenger($container->get('messenger'));
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_perz_export';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $queue_count = intval($this->exportQueue->getQueueCount());
    $form['run_export_queue']['queue-list'] = [
      '#type' => 'item',
      '#title' => $this->t('Number of queue items in the Export Queue'),
      '#description' => $this->t('%num @items.', [
        '%num' => $queue_count,
        '@items' => $queue_count === 1 ? $this->t('item') : $this->t('items'),
      ]),
    ];
    $form['rescan_content'] = [
      '#type' => 'submit',
      '#value' => t('Rescan content'),
    ];
    $form['purge_content'] = [
      '#type' => 'submit',
      '#value' => t('Purge content'),
    ];
    $form['export_content'] = [
      '#type' => 'submit',
      '#value' => t('Export content'),
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $triggered_button = $form_state->getTriggeringElement()['#parents'][0];
    if ($triggered_button === 'rescan_content') {
      $this->exportQueue->rescanContent();
      \Drupal::messenger()->addMessage('All content has been rescanned and added to the Queue.');
    }
    elseif ($triggered_button === 'export_content') {
      $this->exportQueue->exportQueueItems();
      \Drupal::messenger()->addMessage('All content has been exported from the Queue.');
    }
    elseif ($triggered_button === 'purge_content') {
      $this->exportQueue->purgeQueues();
      \Drupal::messenger()->addMessage('All content has been purged from the Queue.');
    }

  }

}
