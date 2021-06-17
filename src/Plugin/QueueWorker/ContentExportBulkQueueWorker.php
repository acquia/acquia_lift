<?php

namespace Drupal\acquia_perz\Plugin\QueueWorker;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Queue\QueueWorkerBase;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\acquia_perz\ExportContent;
use Drupal\Core\Queue\SuspendQueueException;

/**
 * Content export bulk queue worker.
 *
 * @QueueWorker(
 *   id = "acquia_perz_content_export_bulk",
 *   title = "Queue Worker to export bulk entities to CIS."
 * )
 */
class ContentExportBulkQueueWorker extends QueueWorkerBase implements ContainerFactoryPluginInterface {

  const DELETED = 'deleted';

  const EXPORTED = 'exported';

  const FAILED = 'failed';

  /**
   * Publishing actions.
   *
   * @var \Drupal\acquia_perz\ExportContent
   */
  protected $exportContent;

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
   * @param \Drupal\acquia_perz\ExportContent $export_content
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
  public function __construct(ExportContent $export_content, EntityTypeManagerInterface $entity_type_manager, ConfigFactoryInterface $config_factory, array $configuration, $plugin_id, $plugin_definition) {
    $this->exportContent = $export_content;
    $this->entityTypeManager = $entity_type_manager;
    $this->configFactory = $config_factory;
    parent::__construct($configuration, $plugin_id, $plugin_definition);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $container->get('acquia_perz.export_content'),
      $container->get('entity_type.manager'),
      $container->get('config.factory'),
      $configuration,
      $plugin_id,
      $plugin_definition
    );
  }

  /**
   * {@inheritdoc}
   */
  public function processItem($data) {
    $this->exportContent->exportEntities($data['entities']);
    return TRUE;
  }

}
