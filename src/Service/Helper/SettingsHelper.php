<?php

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Component\Utility\UrlHelper;
use GuzzleHttp\Exception\RequestException;

/**
 * Defines the Settings Helper class.
 */
class SettingsHelper {

  /**
   * Default identity type's default value.
   */
  const DEFAULT_IDENTITY_TYPE_DEFAULT = 'email';

  /**
   * Cdf version's default value.
   */
  const CDF_VERSION_DEFAULT = 1;

  /**
   * Is an invalid credential.
   *
   * @param array $credential_settings
   *   Credential settings array.
   *
   * @return bool
   *   True if is an invalid credential.
   */
  public static function isInvalidCredential(array $credential_settings) {
    if (self::isInvalidCredentialAccountId($credential_settings['account_id']) ||
      self::isInvalidCredentialSiteId($credential_settings['site_id']) ||
      self::isInvalidCredentialAssetsUrl($credential_settings['assets_url']) ||
      isset($credential_settings['decision_api_url']) && self::isInvalidCredentialDecisionAPIUrl($credential_settings['decision_api_url'])
    ) {
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Is an invalid credential Account ID.
   *
   * Invalid if:
   *   1) Missing, or
   *   2) Not start with a letter and contain only alphanumerical characters.
   *
   * @param string $account_id
   *   Credential Account ID.
   *
   * @return bool
   *   True if is an invalid credential Account ID.
   */
  public static function isInvalidCredentialAccountId($account_id) {
    if (empty($account_id) || !preg_match('/^[a-zA-Z_][a-zA-Z\\d_]*$/', $account_id)) {
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Is an invalid credential Site ID.
   *
   * Invalid if:
   *   1) Missing, or
   *   2) Not alphanumerical.
   *
   * @param string $site_id
   *   Credential Site ID.
   *
   * @return bool
   *   True if is an invalid credential Site ID.
   */
  public static function isInvalidCredentialSiteId($site_id) {
    if (empty($site_id)) {
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Is an invalid credential Assets URL.
   *
   * Invalid if:
   *   1) Missing, or
   *   2) Not a valid URL.
   *
   * @param string $assets_url
   *   Credential Assets URL.
   *
   * @return bool
   *   True if is an invalid credential Assets URL.
   */
  public static function isInvalidCredentialAssetsUrl($assets_url) {
    if (empty($assets_url) || !UrlHelper::isValid($assets_url)) {
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Is an invalid credential Decision API URL.
   *
   * Invalid if:
   *   1) Exist, and
   *   2) Not a valid URL.
   *
   * @param string $decision_api_url
   *   Credential Decision API URL.
   *
   * @return bool
   *   True if is an invalid credential Decision API URL.
   */
  // phpcs:ignore
  public static function isInvalidCredentialDecisionAPIUrl($decision_api_url) {
    if (!empty($decision_api_url) && !UrlHelper::isValid($decision_api_url)) {
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Is a valid bootstrap mode.
   *
   * @param string $test_mode
   *   Mode to compare.
   *
   * @return bool
   *   True if valid, false otherwise.
   */
  public static function isValidBootstrapMode($test_mode) {
    $valid_modes = ['auto', 'manual'];
    return in_array($test_mode, $valid_modes);
  }

  /**
   * Is a valid content replacement mode.
   *
   * @param string $test_mode
   *   Mode to compare.
   *
   * @return bool
   *   True if valid, false otherwise.
   */
  public static function isValidContentReplacementMode($test_mode) {
    $valid_modes = ['trusted', 'customized'];
    return in_array($test_mode, $valid_modes);
  }

  /**
   * Is a valid cdf version.
   *
   * @param string $version
   *   Version to compare.
   *
   * @return bool
   *   True if valid, false otherwise.
   */
  public static function isValidCdfVersion($version) {
    $valid_versions = [1, 2];
    return in_array($version, $valid_versions);
  }

  /**
   * Returns the list of UDFs that can be mapped to.
   *
   * @param string $type
   *   The type of UDF field. Can be person, touch or event.
   *
   * @return array
   *   An array of possible UDF metatag values for the given type.
   *
   * @throws \Exception
   *   An exception if the type given is not supported.
   */
  public static function getUdfLimitsForType($type = "person") {
    if ($type !== 'person' && $type !== 'touch' && $type !== 'event') {
      throw new \Exception('This UDF Field type is not supported.');
    }
    $counts = [
      'person' => 50,
      'touch' => 20,
      'event' => 50,
    ];
    return $counts[$type];
  }

  /**
   * Ping URI.
   *
   * @param string $base_uri
   *   Base URI.
   * @param string $path
   *   Path to "ping" end point.
   *
   * @return array
   *   Returns 'statusCode' and 'reasonPhrase' of the response.
   */
  public static function pingUri($base_uri, $path) {
    /** @var \Drupal\Core\Http\ClientFactory $clientFactory */
    $clientFactory = \Drupal::service('http_client_factory');
    $client = $clientFactory->fromOptions(['base_uri' => $base_uri]);

    try {
      $response = $client->get($path, ['http_errors' => FALSE]);
    }
    catch (RequestException $e) {
      return [];
    }

    return [
      'statusCode' => $response->getStatusCode(),
      'reasonPhrase' => $response->getReasonPhrase(),
    ];
  }

}
