<?php

namespace Drupal\acquia_lift\Service\Context;

interface ContextInterface {

  /**
   * Populate page by context.
   *
   * @param &$page
   *   The page that is to be populated.
   */
  public function populate(&$page);

}
