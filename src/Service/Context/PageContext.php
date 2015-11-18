<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Context\PageContext.
 */

namespace Drupal\acquia_lift\Service\Context;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Config\ConfigFactoryInterface;

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
  private $pageContext =  array(
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
  );

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entityTypeManager
   *   Entity type manager.
   */
  public function __construct(ConfigFactoryInterface $config_factory, EntityTypeManagerInterface $entity_type_manager) {
    $this->fieldMappings = $config_factory->get('acquia_lift.settings')->get('field_mappings');
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
    //@todo: this needs to be converted to a proper thumbnail_url.
    $this->pageContext['thumbnail_url'] = $node->field_image->entity->url();

    $this->setFields($node);
  }

  /**
   * Set fields.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  private function setFields($node) {
    // Find the node's terms.
    $nids = array($node->id());
    $terms = $this->taxonomyTermStorage->getNodeTerms($nids, $this->fieldMappings);
    $node_terms = $terms[$node->id()];

    // Find the term names.
    $vocabulary_term_names = array();
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
