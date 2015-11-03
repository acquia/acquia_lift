<?php

/**
 * @file
 * Provides an agent type for Acquia Lift Profiles
 */
namespace Drupal\acquia_lift;

use Drupal\acquia_lift\Exception\AcquiaLiftDataConnectorCredsException;
use Drupal\acquia_lift\Exception\AcquiaLiftDataConnectorException;
use Drupal\Component\Utility\SafeMarkup;
use Drupal\Component\Utility\UrlHelper;
use Drupal\Core\Config\ConfigFactory;
use Drupal\Core\Routing\RequestContext;
use GuzzleHttp\ClientInterface;
use GuzzleHttp\Psr7\Request;


class AcquiaLiftDataAPI implements AcquiaLiftDataInterface {

  /**
   * An http client for making calls to Acquia Lift web data collection service.
   *
   * @var \GuzzleHttp\ClientInterface
   */
  protected $httpClient;

  /**
   * The API URL for Acquia Lift Data collection.
   *
   * @var string
   */
  protected $apiUrl;

  /**
   * The Acquia Lift account name to use.
   *
   * @var string
   */
  protected $accountName;

  /**
   * The customer site to use.
   *
   * @var string
   */
  protected $siteName;

  /**
   * The access key to use for authorization.
   *
   * @var string
   */
  protected $accessKey;

  /**
   * The secret key to use for authorization.
   *
   * @var string
   */
  protected $secretKey;

  /**
   * The current Drupal request.
   *
   * @var \Drupal\Core\Routing\RequestContext
   */
  protected $context;

  /**
   * The list of headers that can be used in the canonical request.
   *
   * @var array
   */
  protected $headerWhitelist = array(
    'Accept',
    'Host',
    'User-Agent'
  );

  /**
   * The logger to use for errors and notices.
   *
   * @var \Psr\Log\LoggerInterface;
   */
  protected $logger = NULL;

  /**
   * Constructor.
   * @param ConfigFactory $config_factory
   *   The config factory service
   * @param ClientInterface $http_client
   *   A Guzzle client interface
   * @param RequestContext $context
   *   The current request
   * @throws AcquiaLiftDataConnectorCredsException
   */
  public function __construct(ConfigFactory $config_factory, ClientInterface $http_client, RequestContext $context) {
    $config = $config_factory->get('acquia_lift.settings');
    $this->context = $context;
    $this->logger = \Drupal::logger('acquia_lift');

    $this->accountName = $config->get('account_name');
    $this->apiUrl = $config->get('api_url');
    $this->accessKey = $config->get('access_key');
    $this->secretKey = $config->get('secret_key');
    $this->siteName = $config->get('customer_site');

    // If either account name or API URL is still missing, bail.
    if (empty($this->apiUrl) || empty($this->accountName) || empty($this->accessKey) || empty($this->secretKey)) {
      throw new AcquiaLiftDataConnectorCredsException('Missing acquia_lift data account information.');
    }
    if (!UrlHelper::isValid($this->apiUrl)) {
      throw new AcquiaLiftDataConnectorCredsException('Acquia Lift Data API URL is not a valid URL.');
    }

    $this->httpClient = $http_client;

    $needs_scheme = strpos($this->apiUrl, '://') === FALSE;
    if ($needs_scheme) {
      // Use the same scheme for Acquia Lift Profiles as we are using here.
      $url_scheme = ($this->context->getScheme() == 'https') ? 'https://' : 'http://';
      $this->apiUrl = $url_scheme . $this->apiUrl;
    }
    if (substr($this->apiUrl, -1) === '/') {
      $this->apiUrl = substr($this->apiUrl, 0, -1);
    }
  }

  /**
   * Resets the singleton instance.
   *
   * Used in unit tests.
   */
  public static function reset() {
    // This needs to be thought through.  The D8 service handling ensures that
    // this is a singleton.  The test instance should be created via
    // factory and that means that this functionality is likely moved outside of
    // this class.
    throw new \Exception('Todo: reset API not yet implemented');
  }

  /**
   * Accessor for the accountName property.
   *
   * @return string
   */
  public function getAccountName() {
    return $this->accountName;
  }

  /**
   * Accessor for the apiUrl property.
   *
   * @return string
   */
  public function getApiUrl() {
    return $this->apiUrl;
  }

  /**
   * Returns an http client to use for Acquia Lift Profiles calls.
   *
   * @return \GuzzleHttp\ClientInterface
   */
  protected function httpClient() {
    return $this->httpClient;
  }

  /**
   * Generates an endpoint for a particular section of the Acquia Lift Data API.
   *
   * @param string $path
   *   The endpoint path, e.g. 'segments' or 'events/my-event'
   * @return string
   *   The endpoint to make calls to.
   */
  protected function generateEndpoint($path) {
    return $this->apiUrl . '/dashboard/rest/' . $this->accountName . '/' . $path;
  }

  /**
   * Returns the canonical representation of a request.
   *
   * @param $method
   *   The request method, e.g. 'GET'.
   * @param $path
   *   The path of the request, e.g. '/dashboard/rest/[ACCOUNTNAME]/segments'.
   * @param array $parameters
   *   An array of request parameters.
   * @param array $headers
   *   An array of request headers.
   *
   * @return string
   *   The canonical representation of the request.
   */
  public function canonicalizeRequest($method, $url, $parameters = array(), $headers = array()) {
    $parsed_url = parse_url($url);
    $str = strtoupper($method) . "\n";
    // Certain headers may get added to the actual request so we need to
    // add them here.
    if (!isset($headers['User-Agent'])) {
      $client_config = $this->httpClient()->getConfig();
      $headers['User-Agent'] = $client_config['headers']['User-Agent'];
    }
    if (!isset($headers['Host'])) {
      $headers['Host'] = $parsed_url['host'] . (!empty($parsed_url['port']) ? ':' . $parsed_url['port'] : '');
    }
    // Sort all header names alphabetically.
    $header_names = array_keys($headers);
    uasort($header_names, create_function('$a, $b', 'return strtolower($a) < strtolower($b) ? -1 : 1;'));
    // Add each header (trimmed and lowercased) and value to the string, separated by
    // a colon, and with a new line after each header:value pair.
    foreach ($header_names as $header) {
      if (!in_array($header, $this->headerWhitelist)) {
        continue;
      }
      $str .= trim(strtolower($header)) . ':' . trim($headers[$header]) . "\n";
    }
    // Add the path.
    $str .= $parsed_url['path'];
    // Sort any parameters alphabetically and add them as a querystring to our string.
    if (!empty($parameters)) {
      ksort($parameters);
      $first_param = key($parameters);
      $str .= '?' . $first_param . '=' . array_shift($parameters);
      foreach ($parameters as $key => $value) {
        $str .= '&' . $key . '=' . $value;
      }
    }
    return $str;
  }

  /**
   * Returns a string to use for the 'Authorization' header.
   *
   * @return string
   */
  public function getAuthHeader($method, $path, $parameters = array(), $headers = array()) {
    $canonical = $this->canonicalizeRequest($method, $path, $parameters, $headers);
    $hmac = base64_encode(hash_hmac('sha1', (string) $canonical, $this->secretKey, TRUE));
    return 'HMAC ' . $this->accessKey . ':' . $hmac;
  }

  /**
   * Fetches the available Segment IDs from Acquia Lift
   *
   * @return array
   *   An array of segment IDs that can be used for targeting.
   */
  public function getSegments() {
    // First get our Authorization header.
    $headers = array('Accept' => 'application/json');
    $url = $this->generateEndpoint('segments');
    $auth_header = $this->getAuthHeader('GET', $url, array(), $headers);
    $headers += array('Authorization' => $auth_header);

    $request = new Request('GET', $url, $headers);
    $response = $this->httpClient()->send($request);
    $data = $response->getBody();
    if (empty($data)) {
      return array();
    }
    $data = json_decode($data, TRUE);
    if (is_array($data)) {
      $segments = array_values(array_filter($data));
      return $segments;
    }
    return array();
  }

  /**
   * Saves an event to Acquia Lift Data
   *
   * @param string $event_name
   *   The name of the event.
   * @param string $event_type
   *   The type of event, can be one of 'CAMPAIGN_ACTION', 'CAMPAIGN_CLICK_THROUGH',
   *   'CAMPAIGN_CONVERSION', or 'OTHER' (default).
   *
   * @throws AcquiaLiftDataConnectorException
   */
  public function saveEvent($event_name, $event_type = 'OTHER') {
    // First get our Authorization header.
    $headers = array('Accept' => 'application/json');
    $url = $this->generateEndpoint('events/' . $event_name);
    $auth_header = $this->getAuthHeader('PUT', $url, array('type' => $event_type), $headers);
    $headers += array('Authorization' => $auth_header);

    $request = new Request('PUT', $url . '?type=' . $event_type, $headers);
    $response = $this->httpClient()->send($request);
    $vars = array('@eventname' => $event_name);
    $success_msg = SafeMarkup::format('The event @eventname has been saved to Acquia Lift', $vars);
    $fail_msg = SafeMarkup::format('Could not save event @eventname to Acquia Lift', $vars);
    if ($response->getStatusCode() == 200) {
      $this->logger->info($success_msg);
    }
    else {
      $this->logger->error($fail_msg);
      throw new AcquiaLiftDataConnectorException($fail_msg);
    }
  }

  /**
   * Deletes an event from Acquia Lift Data
   *
   * @param $event_name
   *   The name of the event.
   *
   * @throws AcquiaLiftDataConnectorException
   */
  public function deleteEvent($event_name) {
    // First get our Authorization header.
    $url = $this->generateEndpoint('events/' . $event_name);
    $auth_header = $this->getAuthHeader('DELETE', $url);

    $request = new Request('DELETE', $url, array('Authorization' => $auth_header));
    $response = $this->httpClient()->send($request);

    $vars = array('@eventname' => $event_name);
    $success_msg = SafeMarkup::format('The event @eventname was deleted from Acquia Lift Profiles', $vars);
    $fail_msg = SafeMarkup::format('Could not delete event @eventname from Acquia Lift Profiles', $vars);
    if ($response->getStatusCode() == 200) {
      $this->logger->info($success_msg);
    }
    else {
      $this->logger->error($fail_msg);
      throw new AcquiaLiftDataConnectorException($fail_msg);
    }
  }
}