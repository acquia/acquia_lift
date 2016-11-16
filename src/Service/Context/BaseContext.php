<?php

namespace Drupal\acquia_lift\Service\Context;

abstract class BaseContext implements ContextInterface {

  /**
   * Page context, with default value.
   *
   * @var array
   */
  protected $context = [];

  /**
   * Get the render array for a single meta tag.
   *
   * @param string $name
   *   The name for the meta tag
   * @param string $content
   *   The content for the meta tag
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
   * {@inheritdoc}
   */
  public function populateHtmlHead(&$htmlHead) {
    // Attach Lift's metatags.
    foreach ($this->context as $name => $content) {
      $renderArray = $this->getMetaTagRenderArray($name, $content);
      // To generate meta tags within HTML head, Drupal requires this precise
      // format of render array.
      $htmlHead[] = [$renderArray, $name];
    }
  }

}
