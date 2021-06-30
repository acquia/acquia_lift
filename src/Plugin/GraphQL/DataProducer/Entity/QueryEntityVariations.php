<?php

namespace Drupal\acquia_perz\Plugin\GraphQL\DataProducer\Entity;

use Cassandra\Uuid;
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
use Drupal\field\Entity\FieldConfig;
use Drupal\Core\Datetime\DateFormatterInterface;
use Drupal\Component\Datetime\TimeInterface;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Render\RendererInterface;

/**
 * @DataProducer(
 *   id = "query_entity_variations",
 *   name = @Translation("Load entity variations"),
 *   description = @Translation("Loads a list of entity variations."),
 *   produces = @ContextDefinition("any",
 *     label = @Translation("List of entity variations")
 *   ),
 *   consumes = {
 *     "entity_type_id" = @ContextDefinition("string",
 *       label = @Translation("Entity type Id")
 *     ),
 *     "entity_uuid" = @ContextDefinition("string",
 *       label = @Translation("Entity Uuid")
 *     )
 *   }
 * )
 */
class QueryEntityVariations extends DataProducerPluginBase implements ContainerFactoryPluginInterface {

  /**
   * The renderer.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

  /**
   * The date formatter service.
   *
   * @var \Drupal\Core\Datetime\DateFormatterInterface
   */
  protected $dateFormatter;

  /**
   * The time service.
   *
   * @var \Drupal\Component\Datetime\TimeInterface
   */
  protected $time;

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
      $container->get('renderer'),
      $container->get('date.formatter'),
      $container->get('datetime.time'),
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
   * @param \Drupal\Core\Render\RendererInterface $renderer
   *   The renderer.
   * @param \Drupal\Core\Datetime\DateFormatterInterface $date_formatter
   *   The date formatter service.
   * @param \Drupal\Component\Datetime\TimeInterface $time
   *   The time service.
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
    RendererInterface $renderer,
    DateFormatterInterface $date_formatter,
    TimeInterface $time,
    ImmutableConfig $entity_settings,
    ImmutableConfig $cis_settings,
    EntityTypeManagerInterface $entity_type_manager,
    EntityFieldManagerInterface $entity_field_manager,
    EntityTypeBundleInfoInterface $entity_type_bundle_info,
    EntityRepositoryInterface $repository,
    Connection $database
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition);
    $this->renderer = $renderer;
    $this->dateFormatter = $date_formatter;
    $this->time = $time;
    $this->entitySettings = $entity_settings;
    $this->cisSettings = $cis_settings;
    $this->entityTypeManager = $entity_type_manager;
    $this->entityFieldManager = $entity_field_manager;
    $this->entityTypeBundleInfo = $entity_type_bundle_info;
    $this->repository = $repository;
    $this->database = $database;
  }

  /**
   * @param string $entity_type_id
   * @param string $entity_uuid
   * @param \Drupal\Core\Cache\RefinableCacheableDependencyInterface $metadata
   *
   * @return array|int
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function resolve(
    $entity_type_id,
    $entity_uuid,
    RefinableCacheableDependencyInterface $metadata) {
    $variations = [];
    $entity = $this->repository->loadEntityByUuid($entity_type_id, $entity_uuid);
    $bundle = $entity->bundle();
    $available_view_modes = $this->entitySettings
        ->get("view_modes.{$entity_type_id}.{$bundle}")
      ?? [];

    if (empty($available_view_modes)) {
      throw new \Exception("{$entity_type_id}.{$bundle} is not available for the export.");
    }

    foreach (array_keys($available_view_modes) as $view_mode) {
      foreach ($entity->getTranslationLanguages() as $language) {
        $langcode = $language->getId();
        $translation = $entity->getTranslation($langcode);
        $variations[] = $this->getEntityVariation(
          $translation,
          $view_mode,
          $langcode
        );
      }
    }
    $metadata->addCacheTags($entity->getCacheTags());
    $metadata->addCacheTags($this->entitySettings->getCacheTags());
    $metadata->addCacheContexts($entity->getCacheContexts());
    $metadata->addCacheContexts($this->entitySettings->getCacheContexts());

    return $variations;
  }

  /**
   * Export entity by view mode.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   * @param string $view_mode
   *   The view mode.
   * @param string $langcode
   *   The language code.
   */
  protected function getEntityVariation(EntityInterface $entity, $view_mode, $langcode) {
    $elements = $this->entityTypeManager
      ->getViewBuilder($entity->getEntityTypeId())
      ->view($entity, $view_mode, $langcode);
    $rendered_data = $this->renderer->renderPlain($elements);
    $result = [
      'content_uuid' => $entity->uuid(),
      'account_id' => $this->cisSettings->get('cis.account_id', 'abcf'),
      'content_type' => $entity->getEntityTypeId(),
      'view_mode' => $view_mode,
      'language' => $langcode,
      'number_view' => 0,
      'label' => $entity->label(),
      'updated' => $this->dateFormatter->format($this->time->getCurrentTime(), 'custom', 'Y-m-d\TH:i:s'),
      'rendered_data' => $rendered_data,
    ];
    $taxonomy_relations = $this->getEntityTaxonomyRelations($entity);
    if ($taxonomy_relations) {
      $result['relations'] = $taxonomy_relations;
    }
    return $result;
  }

  /**
   * Returns array of related taxonomy term fields and their corresponding term
   * uuids; only taxonomies that are checked on Entity settings form.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *
   * @return array
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  protected function getEntityTaxonomyRelations(EntityInterface $entity) {
    $relations = [];
    $entity_type_id = $entity->getEntityTypeId();
    $bundle = $entity->bundle();
    $view_modes = $this->entitySettings->get('view_modes');
    $available_taxonomies = [];
    if (isset($view_modes['taxonomy_term'])) {
      $available_taxonomies = array_keys($view_modes['taxonomy_term']);
    }
    $fields = $this->entityFieldManager
      ->getFieldDefinitions($entity_type_id, $bundle);
    foreach ($fields as $field) {
      if ($field instanceof FieldConfig
        && $field->getType() === 'entity_reference'
        && $field->getSetting('handler') === 'default:taxonomy_term'
      ) {
        $field_name = $field->getName();
        $settings = $field->getSetting('handler_settings');
        $field_taxonomies = $settings['target_bundles'];
        // Check if field contains at least one available taxonomy.
        if (count(array_intersect($available_taxonomies, $field_taxonomies)) == 0) {
          continue;
        }
        $terms = $entity->get($field_name)->getValue();
        $available_field_terms = [];
        foreach ($terms as $term) {
          $term_entity = $this->entityTypeManager
            ->getStorage('taxonomy_term')
            ->load($term['target_id']);
          $term_uuid = $term_entity->uuid();
          if (in_array($term_entity->bundle(), $available_taxonomies)) {
            $available_field_terms[] = $term_uuid;
          }
        }
        $relations[] = [
          'field' => $field_name,
          'terms' => $available_field_terms,
        ];
      }
    }
    return $relations;
  }

}
