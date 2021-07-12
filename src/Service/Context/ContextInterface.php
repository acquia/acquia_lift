<?php

namespace Drupal\acquia_lift\Service\Context;

/**
 * The Context interface.
 */
interface ContextInterface {

  /**
   * Populate page by context.
   *
   * @param array &$page
   *   The page that is to be populated.
   */
  public function populate(array &$page);

}
