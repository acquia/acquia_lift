<?php

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Utility\LinkGeneratorInterface;
use Drupal\Core\Url;

class HelpMessageHelper {
  /**
   * Acquia Lift credential settings.
   *
   * @var array
   */
  private $credentialSettings;

  /**
   * Link generator.
   *
   * @var \Drupal\Core\Utility\LinkGeneratorInterface
   */
  private $linkGenerator;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Utility\LinkGeneratorInterface $link_generator
   *   The link generator.
   */
  public function __construct(ConfigFactoryInterface $config_factory, LinkGeneratorInterface $link_generator) {
    $this->credentialSettings = $config_factory->get('acquia_lift.settings')->get('credential');
    $this->linkGenerator = $link_generator;
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
      case 'help.page.acquia_lift':
      case 'acquia_lift.admin_settings_form':
        $link_attributes = ['attributes' => ['target' => '_blank']];

        // Generate Documentation link.
        $documentation_link_text = t('Documentation');
        $documentation_link_url = Url::fromUri('https://docs.acquia.com/lift/', $link_attributes);
        $documentation_external_link = $this->linkGenerator->generate($documentation_link_text, $documentation_link_url);
        $help_message = t('You can find more info in ') . $documentation_external_link;

        // Generate Acquia Lift Web Admin link.
        if (!empty($this->credentialSettings['api_url'])) {
          $lift_web_link_text = t('Acquia Lift Web Admin');
          $lift_web_link_url = Url::fromUri('https://' . $this->credentialSettings['api_url'], $link_attributes);
          $lift_web_external_link = $this->linkGenerator->generate($lift_web_link_text, $lift_web_link_url);
          $help_message .= t(', and control your web services settings at ') . $lift_web_external_link;
        }

        $help_message .= t('.');
        return $help_message;
    }
    return;
  }
}
