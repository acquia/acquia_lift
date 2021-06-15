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
 *       label = @Translation("Tags (array of term uuids)"),
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
 *     ),
 *     "exclude" = @ContextDefinition("any",
 *       label = @Translation("Exclude (array of content uuids)"),
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
   * @param array $exclude
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
    $exclude,
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
      $terms_fields = $this->getTermsFields($tags, $entity_type_id);
      $main_group = $all_tags ?
        $query->andConditionGroup() :
        $query->orConditionGroup();
      foreach ($terms_fields as $term_uuid => $term_fields) {
        $terms_group = $query->orConditionGroup();
        foreach ($term_fields as $term_field_name) {
          $terms_group->condition(
            $query->orConditionGroup()
              ->condition("$term_field_name.entity.uuid", $term_uuid)
          );
        }
        $main_group->condition($terms_group);
      }
      $query->condition($main_group);
    }

    // Exclude content UUIDs filter.
    if (!empty($exclude)) {
      $query->condition('uuid', $exclude, 'NOT IN');
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
        $query->addMetaData('entity_type_id', $entity_type_id);
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
   * Get list of term uuids and corresponding taxonomy name per each term.
   * @param $term_uuids array
   *   The array of term uuids.
   * @return array
   *   Format: [term_uuid => taxonomy_name]
   */
  protected function getTermsTaxonomy($term_uuids) {
    $result = [];
    $entity_repository = \Drupal::service('entity.repository');
    foreach ($term_uuids as $term_uuid) {
      $term = $entity_repository->loadEntityByUuid('taxonomy_term', $term_uuid);
      $taxonomy_name = $term->bundle();
      $result[$term_uuid] = $taxonomy_name;
    }
    return $result;
  }

  /**
   * Get list of taxonomies and corresponding fields per each taxonomy.
   * @param integer $entity_type_id
   *   The entity type id.
   * @return array
   *   Format: [taxonomy => [fields]]
   */
  protected function getTaxonomyFields($entity_type_id) {
    $taxonomy_fields = [];
    $bundles = $this->entityTypeBundleInfo
      ->getBundleInfo($entity_type_id);
    foreach (array_keys($bundles) as $bundle) {
      $fields = $this->entityFieldManager
        ->getFieldDefinitions($entity_type_id, $bundle);
      foreach ($fields as $field) {
        if ($field instanceof FieldConfig
          && $field->getType() === 'entity_reference'
          && $field->getSetting('handler') === 'default:taxonomy_term'
        ) {
          $settings = $field->getSetting('handler_settings');
          // Group fields per corresponding taxonomy.
          $taxonomies = $settings['target_bundles'];
          $field_name = $field->getName();
          foreach ($taxonomies as $taxonomy) {
            if (isset($taxonomy_fields[$taxonomy])) {
              if (in_array($field_name, $taxonomy_fields[$taxonomy])) {
                continue;
              }
              $taxonomy_fields[$taxonomy][] = $field_name;
            }
            else {
              $taxonomy_fields[$taxonomy] = [$field_name];
            }
          }
        }
      }
    }
    return $taxonomy_fields;
  }

  /**
   * Get list of term uuids and corresponding list of fields per each term.
   *
   * @param $term_uuids array
   *   The array of term uuids.
   * @param integer $entity_type_id
   *   The entity type id.
   * @return array
   *   Format: [term_uuid => [fields]]
   */
  protected function getTermsFields($term_uuids, $entity_type_id) {
    $terms_taxonomy = $this->getTermsTaxonomy($term_uuids);
    $taxonomy_fields = $this->getTaxonomyFields($entity_type_id);
    $result = [];
    foreach ($terms_taxonomy as $term_uuid => $taxonomy_name) {
      if (!empty($taxonomy_fields[$taxonomy_name])) {
        $result[$term_uuid] = $taxonomy_fields[$taxonomy_name];
      }
    }
    return $result;
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
