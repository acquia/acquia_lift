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
use Drupal\Core\Entity\EntityRepositoryInterface;

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
      $container->get('entity.repository'),
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
    EntityTypeManagerInterface $entity_type_manager,
    EntityFieldManagerInterface $entity_field_manager,
    EntityTypeBundleInfoInterface $entity_type_bundle_info,
    EntityRepositoryInterface $repository,
    Connection $database
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition);
    $this->entityTypeManager = $entity_type_manager;
    $this->entityFieldManager = $entity_field_manager;
    $this->entityTypeBundleInfo = $entity_type_bundle_info;
    $this->repository = $repository;
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
    $entity_type = $storage->getEntityType();
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
      $label_property = $entity_type->getKey('label');
      $text_fields = $this->getTextConditions($entity_type_id);
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
      $grouped_terms_uuids = $this->categorizeTerms($tags);
      $taxonomy_term_fields = $this->getTaxonomyTermFields($entity_type_id);
      foreach ($taxonomy_term_fields as $taxonomy_term_field) {
        $taxonomy_term_field_name = $taxonomy_term_field->getName();
        $settings = $taxonomy_term_field->getSetting('handler_settings');
        foreach ($settings['target_bundles'] as $taxonomy) {
          if ($all_tags) {
            foreach ($grouped_terms_uuids[$taxonomy] as $term_uuid) {
              $query->condition($query->andConditionGroup()
                ->condition("$taxonomy_term_field_name.entity.uuid", $term_uuid)
              );
            }
          }
          else {
            $query->condition("$taxonomy_term_field_name.entity.uuid", $grouped_terms_uuids[$taxonomy], 'IN');
          }
        }
      }
    }

    // Sorting.
    if (!empty($sort)) {
      $sort_order = empty($sort_order)
      || ($sort_order !== 'asc'
          && $sort_order !== 'desc')
        ? 'desc' : $sort_order;
      if ($sort === 'number_of_views') {
        $query->addMetaData('entity_id_column', $entity_type->getKey('id'));
        $query->addMetaData('sort_order', $sort_order);
        $query->addTag('perz_metric_order');
      }
      elseif (isset($entity_type_definition[$sort])) {
        $sort_order = strtolower($sort_order);
        $query->sort($sort, $sort_order);
      }
    }

    $query->range($start, $rows);

    $metadata->addCacheTags($entity_type->getListCacheTags());
    $metadata->addCacheContexts($entity_type->getListCacheContexts());

    return $query->execute();
  }

  /**
   * Categorize term uuids by its taxonomy.
   *
   * @param array $term_uuids
   *
   * @return array
   *   Format: [taxonomy_name => [term_uuid1,...]]
   */
  protected function categorizeTerms($term_uuids) {
    $result = [];
    foreach ($term_uuids as $term_uuid) {
      $term = $this->repository->loadEntityByUuid('taxonomy_term', $term_uuid);
      $taxonomy_name = $term->bundle();
      if (isset($result[$taxonomy_name])) {
        $result[$taxonomy_name][] = $term_uuid;
      }
      else {
        $result[$taxonomy_name] = [$term_uuid];
      }
    }
    return $result;
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
      $fields = $this->entityFieldManager
        ->getFieldDefinitions($entity_type_id, $bundle);
      foreach ($fields as $field) {
        if ($field instanceof FieldConfig
          && $field->getType() === 'entity_reference'
          && $field->getSetting('handler') === 'default:taxonomy_term'
          && !in_array($field->getName(), $taxonomy_term_fields)) {
          $taxonomy_term_fields[] = $field;
        }
      }
    }
    return $taxonomy_term_fields;
  }

  /**
   * Go through each bundle of entity type and return
   * a list of all text fields condition lines including
   * nested paragraphs.
   *
   * @param integer $entity_type_id
   *   The entity type id.
   *
   * @return array
   */
  protected function getTextConditions($entity_type_id) {
    $bundles = $this->entityTypeBundleInfo
      ->getBundleInfo($entity_type_id);
    $text_fields = [];
    foreach (array_keys($bundles) as $bundle) {
      $text_fields = array_merge(
        $text_fields,
        $this->buildNestedConditions($entity_type_id, $bundle)
      );
    }
    $text_fields = array_unique($text_fields);
    return $text_fields;
  }

  /**
   * Go through all fields of the bundle and build condition lines
   * for text fields.
   * For paragraph reference fields it runs recursively and builds
   * the same.
   *
   * @param string $entity_type_id
   *   The entity type id.
   * @param string $bundle
   *   Bundle name.
   *
   * @return array
   */
  protected function buildNestedConditions($entity_type_id, $bundle) {
    $result = [];
    $fields = $this->entityFieldManager
      ->getFieldDefinitions($entity_type_id, $bundle);
    foreach ($fields as $field) {
      if ($field instanceof FieldConfig) {
        $field_name = $field->getName();
        $field_type = $field->getType();
        // Check that field is a paragraph reference.
        if ($field_type === 'entity_reference_revisions'
          && $field->getSetting('handler') === 'default:paragraph') {
          $handler_settings = $field->getSetting('handler_settings');
          $paragraph_types = $handler_settings['target_bundles'];
          foreach ($paragraph_types as $paragraph_type) {
            $paragraph_result = $this->buildNestedConditions('paragraph', $paragraph_type);
            $condition_line = "$field_name.entity.";
            $result = array_merge(
              $result,
              // Add condition line as prefix to each element of the array.
              preg_filter('/^/', $condition_line, $paragraph_result)
            );
          }
        }
        // Check that field has one of Text related types.
        elseif (in_array($field_type, $this->getFieldTextTypes())) {
          $result[] = $field_name;
        }
      }
    }
    return $result;
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
