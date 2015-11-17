<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Context\PageContext.
 */

namespace Drupal\acquia_lift\Service\Context;

use Drupal\Core\Entity\EntityInterface;

class PageContext {
  /**
   * Engagement score's default value.
   */
  const ENGAGEMENT_SCORE_DEFAULT = 1;

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
    'engagement_score' => self::ENGAGEMENT_SCORE_DEFAULT,
    'author' => '',
    'evalSegments' => TRUE,
    'trackingId' => '',
  );

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
