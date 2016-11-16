<?php

namespace Drupal\acquia_lift\Service\Context;

interface ContextInterface {

  /**
   * Populate page's HTML head.
   *
   * @param &$htmlHead
   *   The HTML head that is to be populated.
   */
  public function populateHtmlHead(&$htmlHead);

}
