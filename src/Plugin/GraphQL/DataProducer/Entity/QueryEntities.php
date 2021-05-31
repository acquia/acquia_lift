<?php

namespace Drupal\acquia_perz\Plugin\GraphQL\DataProducer\Entity;

use Drupal\Core\Cache\RefinableCacheableDependencyInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Entity\EntityFieldManagerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Entity\EntityTypeBundleInfoInterface;
use Drupal\field\Entity\FieldConfig;
use Drupal\Core\Database\Connection;

/**
 * @DataProducer(
 *   id = "query_entities",
 *   name = @Translation("Load entities"),
 *   description = @Translation("Loads a list of entities."),
 *   produces = @ContextDefinition("any",
 *     label = @Translation("Entity connection")
 *   ),
 *   consumes = {
 *     "entity_type_id" = @ContextDefinition("string",
 *       label = @Translation("Entity type Id")
 *     ),
 *     "start" = @ContextDefinition("integer",
 *       label = @Translation("Start"),
 *       required = FALSE
 *     ),
 *     "rows" = @ContextDefinition("integer",
 *       label = @Translation("Rows"),
 *       required = FALSE
 *     ),
 *     "langcode" = @ContextDefinition("string",
 *       label = @Translation("Langcode"),
 *       required = FALSE
 *     ),
 *     "date_start" = @ContextDefinition("string",
 *       label = @Translation("Date start"),
 *       required = FALSE
 *     ),
 *     "date_end" = @ContextDefinition("string",
 *       label = @Translation("Date end"),
 *       required = FALSE
 *     ),
 *     "q" = @ContextDefinition("string",
 *       label = @Translation("Filtering by text"),
 *       required = FALSE
 *     ),
 *     "tags" = @ContextDefinition("any",
 *       label = @Translation("Tags (array of uuids)"),
 *       required = FALSE
 *     ),
 *     "all_tags" = @ContextDefinition("boolean",
 *       label = @Translation("All Tags"),
 *       required = FALSE
 *     ),
 *     "sort" = @ContextDefinition("string",
 *       label = @Translation("Sort (field)"),
 *       required = FALSE
 *     ),
 *     "sort_order" = @ContextDefinition("string",
 *       label = @Translation("Sort Order"),
 *       required = FALSE
 *     )
 *   }
 * )
 */
class QueryEntities extends DataProducerPluginBase implements ContainerFactoryPluginInterface {

  CONST DATE_RANGE_PROPERTY = 'created';

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
   * Database Service Object.
   *
   * @var \Drupal\Core\Database\Connection
   */
  protected $database;

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
      $container->get('entity_type.manager'),
      $container->get('entity_field.manager'),
      $container->get('entity_type.bundle.info'),
      $container->get('database')
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
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Entity\EntityFieldManagerInterface $entity_field_manager
   *   The entity field manager.
   * @param \Drupal\Core\Entity\EntityTypeBundleInfoInterface $entity_type_bundle_info
   *   The entity type bundle info service.
   * @param \Drupal\Core\Database\Connection $database
   *   The database service.
   * @codeCoverageIgnore
   */
  public function __construct(
    array $configuration,
    $pluginId,
    $pluginDefinition,
    EntityTypeManagerInterface $entity_type_manager,
    EntityFieldManagerInterface $entity_field_manager,
    EntityTypeBundleInfoInterface $entity_type_bundle_info,
    Connection $database
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition);
    $this->entityTypeManager = $entity_type_manager;
    $this->entityFieldManager = $entity_field_manager;
    $this->entityTypeBundleInfo = $entity_type_bundle_info;
    $this->database = $database;
  }

  /**
   * @param string $entity_type_id
   * @param int $start
   * @param int $rows
   * @param string $langcode
   * @param string $date_start
   * @param string $date_end
   * @param string $q
   * @param array $tags
   * @param boolean $all_tags
   * @param string $sort
   * @param string $sort_order
   * @param \Drupal\Core\Cache\RefinableCacheableDependencyInterface $metadata
   *
   * @return array|int
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function resolve(
    $entity_type_id,
    $start,
    $rows,
    $langcode,
    $date_start,
    $date_end,
    $q,
    $tags,
    $all_tags,
    $sort,
    $sort_order,
    RefinableCacheableDependencyInterface $metadata) {
    $entity_type_definition = $this->entityFieldManager
      ->getFieldStorageDefinitions($entity_type_id);

    $storage = $this->entityTypeManager
      ->getStorage($entity_type_id);
    $entity_type_data = $storage->getEntityType();
    $query = $storage->getQuery()
      ->currentRevision()
      ->accessCheck();

    // Date range filter.
    if (isset($entity_type_definition[self::DATE_RANGE_PROPERTY])) {
      if (!empty($date_start)) {
        $date_start .= 'T00:00:00';
        $date_start_timestamp = strtotime($date_start);
        $query->condition(self::DATE_RANGE_PROPERTY, $date_start_timestamp, '>');
      }
      if (!empty($date_start)) {
        $date_end .= 'T23:59:59';
        $date_end_timestamp = strtotime($date_end);
        $query->condition(self::DATE_RANGE_PROPERTY, $date_end_timestamp, '<');
      }
    }

    // q filter.
    if (!empty($q)) {
      $label_property = $entity_type_data->getKey('label');
      $text_fields = $this->getTextFields($entity_type_id);
      $or_group = $query->orConditionGroup();
      // Filter by entity label.
      if (!empty($label_property)) {
        $or_group->condition($label_property, "%" . $this->database->escapeLike($q) . "%", 'LIKE');
      }
      // Filter by entity text fields.
      foreach ($text_fields as $text_field) {
        $or_group->condition($text_field, "%" . $this->database->escapeLike($q) . "%", 'LIKE');
      }
      $query->condition($or_group);
    }

    // Taxonomy term filter.
    if (!empty($tags)) {
      $taxonomy_term_fields = $this->getTaxonomyTermFields($entity_type_id);
      foreach ($taxonomy_term_fields as $taxonomy_term_field) {
        if ($all_tags) {
          foreach ($tags as $term_uuid) {
            $query->condition($query->andConditionGroup()
              ->condition("$taxonomy_term_field.entity.uuid", $term_uuid)
            );
          }
        }
        else {
          $query->condition("$taxonomy_term_field.entity.uuid", $tags, 'IN');
        }
      }
    }

    // Sorting.
    if (!empty($sort)
      && isset($entity_type_definition[$sort])
      && !empty($sort_order)) {
      $sort_order = strtolower($sort_order);
      if ($sort_order === 'asc'
        || $sort_order === 'desc') {
        $query->sort($sort, $sort_order);
      }
    }

    $query->range($start, $rows);

    $metadata->addCacheTags($entity_type_data->getListCacheTags());
    $metadata->addCacheContexts($entity_type_data->getListCacheContexts());

    return $query->execute();
  }

  /**
   * Get a list of taxonomy term fields by entity type id.
   *
   * @param integer $entity_type_id
   *   The entity type id.
   *
   * @return array
   */
  protected function getTaxonomyTermFields($entity_type_id) {
    $bundles = $this->entityTypeBundleInfo
      ->getBundleInfo($entity_type_id);
    $taxonomy_term_fields = [];
    foreach (array_keys($bundles) as $bundle) {
      $fields = \Drupal::service('entity_field.manager')
        ->getFieldDefinitions($entity_type_id, $bundle);
      foreach ($fields as $field) {
        if ($field instanceof FieldConfig
          && $field->getType() === 'entity_reference'
          && $field->getSetting('handler') === 'default:taxonomy_term'
          && !in_array($field->getName(), $taxonomy_term_fields)) {
          $taxonomy_term_fields[] = $field->getName();
        }
      }
    }
    return $taxonomy_term_fields;
  }

  /**
   * Get a list of all text fields by entity type id.
   *
   * @param integer $entity_type_id
   *   The entity type id.
   *
   * @return array
   */
  protected function getTextFields($entity_type_id) {
    $text_types = $this->getFieldTextTypes();
    $bundles = $this->entityTypeBundleInfo
      ->getBundleInfo($entity_type_id);
    $text_fields = [];
    foreach (array_keys($bundles) as $bundle) {
      $fields = $this->entityFieldManager
        ->getFieldDefinitions($entity_type_id, $bundle);
      foreach ($fields as $field) {
        if ($field instanceof FieldConfig
          && in_array($field->getType(), $text_types)
          && !in_array($field->getName(), $text_fields)) {
          $text_fields[] = $field->getName();
        }
      }
    }
    return $text_fields;
  }

  /**
   * Get list of text field types.
   *
   * @return string[]
   */
  protected function getFieldTextTypes() {
    return [
      'text',
      'text_long',
      'text_with_summary',
      'string',
      'string_long'
    ];
  }

}
