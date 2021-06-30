<?php

namespace Drupal\acquia_perz\Plugin\GraphQL\DataProducer\Entity;

use Drupal\Core\Cache\RefinableCacheableDependencyInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Entity\EntityFieldManagerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Entity\EntityTypeBundleInfoInterface;
use Drupal\Core\Config\ImmutableConfig;
use Drupal\Core\Database\Connection;
use Drupal\Core\Entity\EntityRepositoryInterface;
use Drupal\Core\Entity\ContentEntityType;

/**
 * @DataProducer(
 *   id = "query_discover_entities",
 *   name = @Translation("Discover all available entities"),
 *   description = @Translation("Return a list of all available entities of different entity types."),
 *   produces = @ContextDefinition("any",
 *     label = @Translation("List of entities")
 *   ),
 *   consumes = {
 *     "page" = @ContextDefinition("integer",
 *       label = @Translation("Pagination: page number")
 *     )
 *   }
 * )
 */
class QueryDiscoverEntities extends DataProducerPluginBase implements ContainerFactoryPluginInterface {

  /**
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * @var \Drupal\Core\Entity\EntityFieldManagerInterface
   */
  protected $entityFieldManager;

  /**
   * The entity type bundle info.
   *
   * @var \Drupal\Core\Entity\EntityTypeBundleInfoInterface
   */
  protected $entityTypeBundleInfo;

  /**
   * The entity repository.
   *
   * @var \Drupal\Core\Entity\EntityRepositoryInterface
   */
  protected $repository;

  /**
   * Database Service Object.
   *
   * @var \Drupal\Core\Database\Connection
   */
  protected $database;

  /**
   * The acquia perz entity settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_perz\Form\ContentPublishingForm
   */
  protected $entitySettings;

  /**
   * The CIS settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_perz\Form\ContentPublishingForm
   */
  protected $cisSettings;

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
      $container->get('acquia_perz.entity_settings'),
      $container->get('acquia_perz.cis_settings'),
      $container->get('entity_type.manager'),
      $container->get('entity_field.manager'),
      $container->get('entity_type.bundle.info'),
      $container->get('entity.repository'),
      $container->get('database')
    );
  }

  /**
   * Constructor.
   *
   * @param array $configuration
   *   The plugin configuration.
   * @param string $pluginId
   *   The plugin id.
   * @param mixed $pluginDefinition
   *   The plugin definition.
   * @param \Drupal\Core\Config\ImmutableConfig $entity_settings
   *   The acquia perz entity settings.
   * @param \Drupal\Core\Config\ImmutableConfig $cis_settings
   *   The cis settings.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Entity\EntityFieldManagerInterface $entity_field_manager
   *   The entity field manager.
   * @param \Drupal\Core\Entity\EntityTypeBundleInfoInterface $entity_type_bundle_info
   *   The entity type bundle info service.
   * @param \Drupal\Core\Entity\EntityRepositoryInterface $repository
   *   The entity repository service.
   * @param \Drupal\Core\Database\Connection $database
   *   The database service.
   * @codeCoverageIgnore
   */
  public function __construct(
    array $configuration,
    $pluginId,
    $pluginDefinition,
    ImmutableConfig $entity_settings,
    ImmutableConfig $cis_settings,
    EntityTypeManagerInterface $entity_type_manager,
    EntityFieldManagerInterface $entity_field_manager,
    EntityTypeBundleInfoInterface $entity_type_bundle_info,
    EntityRepositoryInterface $repository,
    Connection $database
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition);
    $this->entitySettings = $entity_settings;
    $this->cisSettings = $cis_settings;
    $this->entityTypeManager = $entity_type_manager;
    $this->entityFieldManager = $entity_field_manager;
    $this->entityTypeBundleInfo = $entity_type_bundle_info;
    $this->repository = $repository;
    $this->database = $database;
  }

  /**
   * Resolver.
   * @param integer $page
   *   Pagination: current page number.
   * @param \Drupal\Core\Cache\RefinableCacheableDependencyInterface $metadata
   *
   * @return array
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function resolve($page, RefinableCacheableDependencyInterface $metadata) {
    $entity_items = [];
    // Number of entities that can be present on one page.
    $page_max_size = 3;
    // Global offset for entities of all available
    // entity types.
    // It will be updated within each iteration.
    $offset = $page_max_size * $page;
    // Global limit for entities of all available
    // entity types.
    // It will be updated within each iteration.
    $limit = $page_max_size;
    $entity_types = $this->entitySettings->get('view_modes');
    if (empty($entity_types)) {
      throw new \Exception("No entity types are available for the export.");
    }
    foreach ($entity_types as $entity_type_id => $bundles) {
      // Number of all available entities by entity type.
      $et_all_entities_count = $this->getCountByEntityTypeId($entity_type_id, $bundles);
      // Number of available entities by entity type
      // that restricted by current Offset and Limit
      // parameters.
      $et_left_entities_count = $this->getCountByEntityTypeId($entity_type_id, $bundles, $offset, $limit);
      $entity_type = $this->entityTypeManager
        ->getStorage($entity_type_id)
        ->getEntityType();
      // This means that all entities of current entity type
      // is not available because of offset. So skip current
      // entity type in this case and decrease offset value
      // that will be used for next entity type's iterations.
      if (!$et_left_entities_count) {
        $offset = ($offset > $et_all_entities_count) ?
          $offset - $et_all_entities_count : 0;
        continue;
      }

      // Get entities by current entity type, its bundles,
      // global limit and global offset.
      $entity_items = array_merge(
        $this->getEntitiesUuids(
          $entity_type_id,
          $bundles,
          $limit,
          $offset
        ),
        $entity_items
      );
      // Update global limit and offset.
      $offset = ($offset > $limit) ? $offset - $limit : 0;
      $limit = $limit - $et_left_entities_count;

      // Stop searching new entities if limit is
      // not positive.
      if ($limit <= 0) {
        break;
      }

      $metadata->addCacheTags($entity_type->getListCacheTags());
      $metadata->addCacheContexts($entity_type->getListCacheContexts());
    }
    $metadata->addCacheTags($this->entitySettings->getCacheTags());
    $metadata->addCacheContexts($this->entitySettings->getCacheContexts());

    // Calculate Page info payload.
    $total_count = $this->getTotalCount($entity_types);
    $pages_number_raw = ($total_count / $page_max_size) - 1;
    $pages_number_rounded = floor($pages_number_raw);
    $pages_number = $pages_number_rounded;
    if ($pages_number_rounded < $pages_number_raw) {
      ++$pages_number;
    }

    return [
      'page_info' => [
        'total_count' => $total_count,
        'current_page_count' => count($entity_items),
        'current_page' => $page,
        'next_page' => ($page < $pages_number) ? $page + 1 : NULL,
        'prev_page' => ($page > 0) ? $page - 1 : NULL,
      ],
      'items' => $entity_items,
    ];
  }

  protected function getEntitiesUuids($entity_type_id, $bundles, $current_limit, $offset) {
    $entity_ids = $this->getEntitiesIds($entity_type_id, $bundles, $current_limit, $offset);
    $entity_type = $this->entityTypeManager
      ->getStorage($entity_type_id)
      ->getEntityType();
    $entity_items = [];
    foreach ($entity_ids as $entity_id) {
      $entity_uuid = $this->getEntityUuidById($entity_type, $entity_id);
      $entity_items[] = [
        'entity_type_id' => $entity_type_id,
        'entity_uuid' => $entity_uuid,
      ];
    }
    return $entity_items;
  }

  protected function getCountByEntityTypeId($entity_type_id, $bundles, $offset = NULL, $limit = NULL) {
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
      return [];
    }
    $bundle_property_name = $this
      ->entityTypeManager
      ->getStorage($entity_type_id)
      ->getEntityType()
      ->getKey('bundle');
    $query = $this
      ->entityTypeManager
      ->getStorage($entity_type_id)
      ->getQuery();
    // For single-bundle entity types like 'user'
    // we don't use bundle related property.
    if (!empty($bundle_property_name)) {
      $query = $query->condition($bundle_property_name, $available_bundles, 'IN');
    }
    if ($offset && $limit) {
      $query->range($offset, $limit);
    }
    return $query->count()->execute();
  }

  protected function getTotalCount($entity_types) {
    $total_count = 0;
    foreach ($entity_types as $entity_type_id => $bundles) {
      $total_count += $this->getCountByEntityTypeId($entity_type_id, $bundles);
    }
    return $total_count;
  }

  /**
   * Returns entity ids by entity type id and passed bundles.
   *
   * @param string $entity_type_id
   *  The entity type id.
   * @param array $bundles
   *  List of bundles of entity type.
   * @param integer $current_page
   *  List of bundles of entity type.
   * @param integer $bundles
   *  List of bundles of entity type.
   */
  protected function getEntitiesIds($entity_type_id, $bundles, $limit, $offset) {
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
      return [];
    }
    $bundle_property_name = $this
      ->entityTypeManager
      ->getStorage($entity_type_id)
      ->getEntityType()
      ->getKey('bundle');
    $query = $this
      ->entityTypeManager
      ->getStorage($entity_type_id)
      ->getQuery();
    // For single-bundle entity types like 'user'
    // we don't use bundle related property.
    if (!empty($bundle_property_name)) {
      $query = $query->condition($bundle_property_name, $available_bundles, 'IN');
    }
    $query->range($offset, $limit);
    return array_values($query->execute());
  }

  /**
   * Get single entity uuid by entity id and its entity type.
   *
   * @param \Drupal\Core\Entity\Annotation\ContentEntityType $entity_type
   *   Entity type object.
   * @param integer $entity_id
   *   The entity id.
   * @return string
   *   The entity uuid.
   */
  protected function getEntityUuidById(ContentEntityType $entity_type, $entity_id) {
    $query = $this->database->select($entity_type->get('base_table'), 't');
    $query->addField('t', 'uuid');
    $query->condition($entity_type->getKey('id'), $entity_id);
    $query->range(0, 1);
    return $query->execute()->fetchField();
  }

}
