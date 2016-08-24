<?php

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\acquia_lift\AcquiaLiftException;
use Drupal\Core\Config\ConfigFactoryInterface;
use Acquia\LiftClient\Lift;

/**
 * Contains helper methods for working with Solr.
 */
class LiftAPIHelper {

  /**
   * The Lift API SDK Client.
   *
   * @var \Acquia\LiftClient\Lift
   */
  protected $lift;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   *
   * @throws \Drupal\acquia_lift\AcquiaLiftException
   *   The reason why it couldn't create the helper.
   */
  public function __construct(ConfigFactoryInterface $config_factory) {
    $settings = $config_factory->get('acquia_lift.settings');
    $credential_settings = $settings->get('credential');

    if (!isset($credential_settings['account_id'])) {
      throw new AcquiaLiftException(
        'Account ID not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['site_id'])) {
      throw new AcquiaLiftException(
        'Site ID not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['public_key'])) {
      throw new AcquiaLiftException(
        'Public Key not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['secret_key'])) {
      throw new AcquiaLiftException(
        'Secret Key not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['decision_api_url'])) {
      throw new AcquiaLiftException(
        'Decision API URL not found. Please verify your settings.'
      );
    }
    $this->lift = new Lift(
      $credential_settings['account_id'],
      $credential_settings['site_id'],
      $credential_settings['public_key'],
      $credential_settings['secret_key'],
      array('base_url' => $credential_settings['decision_api_url'])
    );
  }

  /**
   * Sets the solr connection.
   *
   * @param \Acquia\LiftClient\Lift $lift
   *   The lift connection object.
   */
  public function setLiftClient(Lift $lift) {
    $this->lift = $lift;
  }

  /**
   * Gets the lift client.
   *
   * @return \Acquia\LiftClient\Lift $lift
   *   The lift connection object.
   */
  public function getLiftClient() {
    return $this->lift;
  }

}
