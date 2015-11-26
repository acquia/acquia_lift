<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Context\PageContext.
 */

namespace Drupal\acquia_lift\Service\Context;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\image\Entity\ImageStyle;

class PageContext {
  /**
   * Engagement score's default value.
   */
  const ENGAGEMENT_SCORE_DEFAULT = 1;

  /**
   * Field mappings.
   *
   * @var array
   */
  private $fieldMappings;

  /**
   * Thumbnail config.
   *
   * @var array
   */
  private $thumbnailConfig;

  /**
   * Taxonomy term storage.
   *
   * @var \Drupal\Core\Entity\EntityStorageInterface
   */
  private $taxonomyTermStorage;

  /**
   * Page context.
   *
   * @var array
   */
  private $pageContext = [
    'content_title' => 'Untitled',
    'content_type' => 'page',
    'page_type' => 'content page',
    'content_section' => '',
    'content_keywords' => '',
    'post_id' => '',
    'published_date' => '',
    'thumbnail_url' => '',
    'persona' => '',
    'engagement_score' => SELF::ENGAGEMENT_SCORE_DEFAULT,
    'author' => '',
    'evalSegments' => TRUE,
    'trackingId' => '',
  ];

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entityTypeManager
   *   Entity type manager.
   */
  public function __construct(ConfigFactoryInterface $config_factory, EntityTypeManagerInterface $entity_type_manager) {
    $settings = $config_factory->get('acquia_lift.settings');
    $this->fieldMappings = $settings->get('field_mappings');
    $this->thumbnailConfig = $settings->get('thumbnail');
    $this->taxonomyTermStorage = $entity_type_manager->getStorage('taxonomy_term');
  }

  /**
   * Set page context.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  public function set(EntityInterface $node) {
    $this->pageContext['content_type'] = $node->getType();
    $this->pageContext['content_title'] = $node->getTitle();
    $this->pageContext['published_date'] = $node->getCreatedTime();
    $this->pageContext['post_id'] = $node->id();
    $this->pageContext['author'] = $node->getOwner()->getUsername();
    $this->pageContext['page_type'] = 'node page';

    $this->setThumbnailUrl($node);
    $this->setFields($node);
  }

  /**
   * Set thumbnail URL.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  private function setThumbnailUrl(EntityInterface $node) {
    $node_type = $node->getType();

    // Don't set, if no thumbnail has been configured.
    if (!isset($this->thumbnailConfig[$node_type])) {
      return;
    }
    $node_type_thumbnail_config = $this->thumbnailConfig[$node_type];
    $thumbnail_config_array = explode('->', $node_type_thumbnail_config['field']);

    $entity = $node;
    foreach ($thumbnail_config_array as $field_key) {
      // Don't set, if node has no such field or field has no such entity.
      if (empty($entity->{$field_key}->entity) ||
        $entity->{$field_key}->isEmpty()
      ) {
        return;
      }
      $entity = $entity->{$field_key}->entity;
    }

    if ($entity->bundle() !== 'file') {
      return;
    }
    $fileUri = $entity->getFileUri();

    // Process Image style.
    $image_style = ImageStyle::load($node_type_thumbnail_config['style']);
    // Return empty if no such image style.
    if (empty($image_style)) {
      return;
    }

    // Generate image URL.
    $thumbnail_uri = $image_style->buildUrl($fileUri);
    $this->pageContext['thumbnail_url'] = file_create_url($thumbnail_uri);
  }

  /**
   * Set fields.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  private function setFields(EntityInterface $node) {
    // Find the node's terms.
    $nids = [$node->id()];
    $terms = $this->taxonomyTermStorage->getNodeTerms($nids, $this->fieldMappings);
    $node_terms = isset($terms[$node->id()]) ? $terms[$node->id()] : [];

    // Find the term names.
    $vocabulary_term_names = [];
    foreach ($node_terms as $term) {
      $vocabulary_id = $term->getVocabularyId();
      $term_name = $term->getName();
      $vocabulary_term_names[$vocabulary_id][] = $term_name;
    }

    // Set the page context.
    foreach ($this->fieldMappings as $page_context_name => $vocabulary_id) {
      if(!isset($vocabulary_term_names[$vocabulary_id])) {
        continue;
      }
      $this->pageContext[$page_context_name] = implode(',', $vocabulary_term_names[$vocabulary_id]);
    }
  }

  /**
   * Get all.
   *
   * @return array
   *   Get All.
   */
  public function getAll() {
    return $this->pageContext;
  }
}
