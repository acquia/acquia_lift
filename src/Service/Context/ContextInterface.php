<?php

namespace Drupal\acquia_lift\Service\Context;

/**
 * Defines the ContextInterface interface.
 */
interface ContextInterface {

  /**
   * Populate page by context.
   *
   * @param array $page
   *   The page that is to be populated.
   */
  public function populate(array &$page);

}
