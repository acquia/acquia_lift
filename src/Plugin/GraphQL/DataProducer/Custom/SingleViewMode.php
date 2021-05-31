<?php

namespace Drupal\acquia_perz\Plugin\GraphQL\DataProducer\Custom;

use Drupal\Core\Cache\RefinableCacheableDependencyInterface;
use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;
use Symfony\Component\HttpFoundation\RequestStack;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Render\RendererInterface;
use Drupal\image\Entity\ImageStyle;

/**
 * @DataProducer(
 *   id = "single_view_mode",
 *   name = @Translation("Single view mode"),
 *   description = @Translation("Simple producer that returns taken parameter."),
 *   produces = @ContextDefinition("any",
 *     label = @Translation("Just returned parameter")
 *   ),
 *   consumes = {
 *     "entity" = @ContextDefinition("entity",
 *       label = @Translation("Entity")
 *     ),
 *     "view_mode" = @ContextDefinition("string",
 *       label = @Translation("View mode")
 *     ),
 *     "context_language" = @ContextDefinition("string",
 *       label = @Translation("Context Language")
 *     )
 *   }
 * )
 */
class SingleViewMode extends DataProducerPluginBase implements ContainerFactoryPluginInterface {

  /**
   * The entity type manager service.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The renderer.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

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
      $container->get('renderer')
    );
  }

  /**
   * EntityLoad constructor.
   *
   * @param array $configuration
   *   The plugin configuration array.
   * @param string $pluginId
   *   The plugin id.
   * @param array $pluginDefinition
   *   The plugin definition array.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entityTypeManager
   *   The entity manager service.
   * @param \Drupal\Core\Render\RendererInterface $renderer
   *   The renderer service.
   *
   * @codeCoverageIgnore
   */
  public function __construct(
    array $configuration,
    $pluginId,
    array $pluginDefinition,
    EntityTypeManagerInterface $entityTypeManager,
    RendererInterface $renderer
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition);
    $this->entityTypeManager = $entityTypeManager;
    $this->renderer = $renderer;
  }

  /**
   * @param \Drupal\Core\Entity\EntityInterface $entity
   * @param $view_mode
   * @param $context_language
   *
   * @return array
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function resolve(EntityInterface $entity, $view_mode, $context_language) {
    $entity_type = $entity->getEntityTypeId();
    if ($view_mode !== 'default') {
      $view_mode_data = $this->entityTypeManager
        ->getStorage('entity_view_mode')->load("$entity_type.$view_mode");
      $view_mode_label = $view_mode_data->label();
    }
    else {
      $view_mode_label = $this->t('Default');
    }

    $elements = $this->entityTypeManager
      ->getViewBuilder($entity_type)
      ->view($entity, $view_mode, $context_language);

    $preview_image_style_id = 'acquia_perz_preview_image';
    $uri = '';
    $preview_image_url = NULL;
    if ($entity->hasField('field_image')) {
      $file_entity = $entity->get('field_image')->entity;
      if (!empty($file_entity->uri)
        && !empty($file_entity->uri->value)) {
        $uri = $entity->get('field_image')->entity->uri->value;
      }
    }
    if (!empty($uri)) {
      $preview_image_style = ImageStyle::load($preview_image_style_id);
      $preview_image_url = $preview_image_style->buildUrl($uri);
    }
    return [
      'id' => $view_mode,
      'label' => $view_mode_label,
      'language' => $context_language,
      'html' => $this->renderer->renderPlain($elements),
      'preview_image' => $preview_image_url,
    ];
  }

}
