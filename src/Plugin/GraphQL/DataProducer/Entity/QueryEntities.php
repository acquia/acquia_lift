<?php

namespace Drupal\acquia_perz\Plugin\GraphQL\DataProducer\Entity;

use Drupal\Core\Cache\RefinableCacheableDependencyInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * @DataProducer(
 *   id = "query_entities",
 *   name = @Translation("Load entities"),
 *   description = @Translation("Loads a list of entities."),
 *   produces = @ContextDefinition("any",
 *     label = @Translation("Entity connection")
 *   ),
 *   consumes = {
 *     "start" = @ContextDefinition("integer",
 *       label = @Translation("Start"),
 *       required = FALSE
 *     ),
 *     "rows" = @ContextDefinition("integer",
 *       label = @Translation("Rows"),
 *       required = FALSE
 *     )
 *   }
 * )
 */
class QueryEntities extends DataProducerPluginBase implements ContainerFactoryPluginInterface {

  /**
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * {@inheritdoc}
   *
   * @codeCoverageIgnore
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $container->get('entity_type.manager')
    );
  }

  /**
   * Articles constructor.
   *
   * @param array $configuration
   *   The plugin configuration.
   * @param string $pluginId
   *   The plugin id.
   * @param mixed $pluginDefinition
   *   The plugin definition.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entityTypeManager
   *
   * @codeCoverageIgnore
   */
  public function __construct(
    array $configuration,
    $pluginId,
    $pluginDefinition,
    EntityTypeManagerInterface $entityTypeManager
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition);
    $this->entityTypeManager = $entityTypeManager;
  }

  /**
   * @param int $start
   * @param int $rows
   * @param \Drupal\Core\Cache\RefinableCacheableDependencyInterface $metadata
   *
   * @return \Drupal\ac_graphql_test\Wrappers\QueryConnection
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function resolve($start, $rows, RefinableCacheableDependencyInterface $metadata) {
    $storage = $this->entityTypeManager->getStorage('node');
    $entityType = $storage->getEntityType();
    $query = $storage->getQuery()
      ->currentRevision()
      ->accessCheck();
    $query->condition($entityType->getKey('bundle'), 'article');
    $query->range($start, $rows);

    $metadata->addCacheTags($entityType->getListCacheTags());
    $metadata->addCacheContexts($entityType->getListCacheContexts());

    return $query->execute();
  }

}
