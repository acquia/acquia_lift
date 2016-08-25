<?php

namespace Drupal\acquia_lift\Lift;

use Drupal\acquia_lift\Exception\APILoaderException;
use Drupal\Core\Config\ConfigFactoryInterface;
use Acquia\LiftClient\Lift;

/**
 * Contains the wrapper around loading the Lift API Client.
 */
class APILoader {

  /**
   * The Lift API SDK Client.
   *
   * @var \Acquia\LiftClient\Lift
   */
  protected $lift;

  /**
   * The config factory
   *
   * @var \Drupal\Core\Config\ConfigFactoryInterface
   */
  protected $configFactory;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   */
  public function __construct(ConfigFactoryInterface $config_factory) {
    $this->configFactory = $config_factory;
  }

  /**
   * Initializes the connection by setting the credentials and the API URL.
   *
   *  @throws \Drupal\acquia_lift\AcquiaLiftException
   *   The reason why it couldn't initialize.
   */
  public function initialize() {
    $settings = $this->configFactory->get('acquia_lift.settings');
    $credential_settings = $settings->get('credential');

    if (!isset($credential_settings['account_id'])) {
      throw new APILoaderException(
        'Account ID not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['site_id'])) {
      throw new APILoaderException(
        'Site ID not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['public_key'])) {
      throw new APILoaderException(
        'Public Key not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['secret_key'])) {
      throw new APILoaderException(
        'Secret Key not found. Please verify your settings.'
      );
    }
    if (!isset($credential_settings['decision_api_url'])) {
      throw new APILoaderException(
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
   *
   *  @throws \Drupal\acquia_lift\AcquiaLiftException
   *   The reason why it could not initialize.
   */
  public function getLiftClient() {
    if (empty($this->lift)) {
      $this->initialize();
    }
    return $this->lift;
  }

}
