<?php

namespace Drupal\acquia_lift\Service\Context;

/**
 * Context Base Class.
 */
abstract class BaseContext implements ContextInterface {

  /**
   * HTML head contexts.
   *
   * @var array
   */
  protected $htmlHeadContexts = [];

  /**
   * Cache contexts.
   *
   * @var array
   */
  protected $cacheContexts = [];

  /**
   * Get the render array for a single meta tag.
   *
   * @param string $name
   *   The name for the meta tag.
   * @param string $content
   *   The content for the meta tag.
   *
   * @return array
   *   The render array
   */
  protected function getMetaTagRenderArray($name, $content) {
    return [
      '#type' => 'html_tag',
      '#tag' => 'meta',
      '#attributes' => [
        'itemprop' => 'acquia_lift:' . $name,
        'content' => $content,
      ],
    ];
  }

  /**
   * Populate page's HTML head.
   *
   * @param array &$html_head
   *   The page's HTML head that is to be populated.
   */
  protected function populateHtmlHead(array &$html_head) {
    // Attach Lift's metatags.
    foreach ($this->htmlHeadContexts as $name => $context) {
      $renderArray = $this->getMetaTagRenderArray($name, $context);
      // To generate meta tags within HTML head, Drupal requires this precise
      // format of render array.
      $html_head[] = [$renderArray, $name];
    }
  }

  /**
   * Populate page's cache context.
   *
   * @param array &$page
   *   The page that is to be populated.
   */
  protected function populateCache(array &$page) {
    // Set cache contexts.
    foreach ($this->cacheContexts as $context) {
      $page['#cache']['contexts'][] = $context;
    }

    // Guard from a possible case that cache contexts contain duplicate items.
    if (isset($page['#cache']['contexts'])) {
      $page['#cache']['contexts'] = array_unique($page['#cache']['contexts']);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function populate(&$page) {
    $this->populateHtmlHead($page['#attached']['html_head']);
    $this->populateCache($page);
  }

}
