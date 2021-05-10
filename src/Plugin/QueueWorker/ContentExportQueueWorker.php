<?php

namespace Drupal\acquia_perz\Plugin\QueueWorker;

use Drupal\Component\Uuid\Uuid;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Logger\LoggerChannelFactoryInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Queue\QueueWorkerBase;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;

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
  public function __construct(EventDispatcherInterface $dispatcher, EntityTypeManagerInterface $entity_type_manager, ConfigFactoryInterface $config_factory, array $configuration, $plugin_id, $plugin_definition) {
    $this->entityTypeManager = $entity_type_manager;
    $this->configFactory = $config_factory;
    parent::__construct($configuration, $plugin_id, $plugin_definition);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
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
    $storage = $this->entityTypeManager->getStorage($data->type);
    $entity = $storage->loadByProperties(['uuid' => $data->uuid]);

    $entity = reset($entity);
    $entities = [];
    $entity_uuids = [];



  }

}
