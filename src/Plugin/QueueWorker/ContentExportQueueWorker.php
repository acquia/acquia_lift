<?php

namespace Drupal\acquia_perz\Plugin\QueueWorker;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Queue\QueueWorkerBase;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\acquia_perz\ContentPublishingActions;

/**
 * Content export queue worker.
 *
 * @QueueWorker(
 *   id = "acquia_perz_publish_export",
 *   title = "Queue Worker to export entities to CIS."
 * )
 */
class ContentExportQueueWorker extends QueueWorkerBase implements ContainerFactoryPluginInterface {

  /**
   * Publishing actions.
   *
   * @var \Drupal\acquia_perz\ContentPublishingActions
   */
  protected $publishingActions;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The config factory.
   *
   * @var \Drupal\Core\Config\ConfigFactoryInterface
   */
  protected $configFactory;

  /**
   * ContentExportQueueWorker constructor.
   *
   * @param \Drupal\acquia_perz\ContentPublishingActions $publishing_actions
   *   The publishing actions service.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory.
   * @param array $configuration
   *   The plugin configuration.
   * @param string $plugin_id
   *   The plugin id.
   * @param mixed $plugin_definition
   *   The plugin definition.
   *
   * @throws \Exception
   */
  //
  public function __construct(ContentPublishingActions $publishing_actions, EntityTypeManagerInterface $entity_type_manager, ConfigFactoryInterface $config_factory, array $configuration, $plugin_id, $plugin_definition) {
    $this->publishingActions = $publishing_actions;
    $this->entityTypeManager = $entity_type_manager;
    $this->configFactory = $config_factory;
    parent::__construct($configuration, $plugin_id, $plugin_definition);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $container->get('acquia_perz.publishing_actions'),
      $container->get('entity_type.manager'),
      $container->get('config.factory'),
      $configuration,
      $plugin_id,
      $plugin_definition
    );
  }

  /**
   * {@inheritdoc}
   *
   * This method return values will be used within ContentHubExportQueue.
   * Different return values will log different messages and will indicate
   * different behaviours:
   *   return FALSE; => Error processing entities, queue item not deleted.
   *   return 0; => No processing done, queue item is not deleted
   *   return TRUE or return int which is not 0 =>
   *      Entities processed and queue item will be deleted.
   */
  public function processItem($data) {
    $entity = $this->entityTypeManager
      ->getStorage($data['entityType'])->load($data['entityId']);
    // Entity missing so remove it from the tracker and stop processing.
    if (!$entity) {
      \Drupal::logger('error')->notice('<pre>'.sprintf(
          'Entity ("%s", "%s") being exported no longer exists on the publisher. Deleting item from the publisher queue.',
          $data['entityType'],
          $data['entityId']
        ).'</pre>');

      return TRUE;
    }
    $this->publishingActions->publishEntityById(
      $data['entityType'],
      $data['entityId']
    );
    return TRUE;
  }

}
