<?php

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Core\StringTranslation\StringTranslationTrait;

/**
 * Help Message Class.
 */
class HelpMessageHelper {

  use StringTranslationTrait;

  /**
   * Get help message (by route name).
   *
   * @param string $route_name
   *   Route name.
   *
   * @return \Drupal\Core\StringTranslation\TranslatableMarkup
   *   The help message.
   */
  public function getMessage($route_name) {
    switch ($route_name) {
      case 'help.page.acquia_lift':
      case 'acquia_lift.admin_settings_form':
        return $this->t('You can find more info in <a href="https://docs.acquia.com/lift" target="_blank">Documentation</a>.');
    }
  }

}
