<?php

namespace Drupal\acquia_lift\Service\Helper;

class HelpMessageHelper {
  /**
   * Get help message (by route name).
   *
   * @param string $route_name
   *   Route name.
   *
   * @return string
   *   The help message.
   */
  public function getMessage($route_name) {
    switch ($route_name) {
      case 'help.page.acquia_lift':
      case 'acquia_lift.admin_settings_form':
        return t('You can find more info in <a href="https://docs.acquia.com/lift" target="_blank">Documentation</a>.');
    }
    return;
  }
}
