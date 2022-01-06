<?php

namespace Drupal\acquia_lift\Service\Context;

/**
 * Acquia Lift Context Interface.
 */
interface ContextInterface {

  /**
   * Populate page by context.
   *
   * @param string &$page
   *   The page that is to be populated.
   */
  public function populate(&$page);

}
