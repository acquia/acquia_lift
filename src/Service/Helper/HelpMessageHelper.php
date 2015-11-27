<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Helper\HelpMessageHelper.
 */

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Url;
use Drupal\acquia_lift\Entity\Credential;

class HelpMessageHelper {
  /**
   * Acquia Lift credential.
   *
   * @var \Drupal\acquia_lift\Entity\Credential
   */
  private $credential;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   */
  public function __construct(ConfigFactoryInterface $config_factory) {
    $credential_settings = $config_factory->get('acquia_lift.settings')->get('credential');
    $this->credential = new Credential($credential_settings);
  }

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
      case 'acquia_lift.admin_settings_form':
        $link_attributes = ['attributes' => ['target' => '_blank']];

        // Generate Documentation link.
        $documentation_link_text = t('Documentation');
        $documentation_link_url = Url::fromUri('https://docs.acquia.com/lift/', $link_attributes);
        $documentation_external_link = \Drupal::l($documentation_link_text, $documentation_link_url);
        $help_message = t('You can find more info in ') . $documentation_external_link;

        // Generate Acquia Lift Web Admin link.
        $api_url = $this->credential->get('api_url');
        if (!empty($api_url)) {
          $lift_web_link_text = t('Acquia Lift Web Admin');
          $lift_web_link_url = Url::fromUri('https://' . $api_url, $link_attributes);
          $lift_web_external_link = \Drupal::l($lift_web_link_text, $lift_web_link_url);
          $help_message .= t(', and control your web services settings at ') . $lift_web_external_link;
        }

        $help_message .= t('.');
        return $help_message;
    }
    return;
  }
}
