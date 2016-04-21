<?php

class AcquiaLiftAPI implements AcquiaLiftLearnReportDataSourceInterface {

  const NAME_MAX_LENGTH = 255;

  /**
   * The Acquia Lift API url to use.
   *
   * @var string
   */
  protected $api_url;

  /**
   * The Acquia Lift Public API Key.
   *
   * @var string
   */
  protected $api_public_key;

  /**
   * The Acquia Lift Private API Key.
   *
   * @var string
   */
  protected $api_private_key;

  /**
   * The Acquia Lift Version we are using
   *
   * @var string
   */
  protected $lift_version;

  /**
   * Whether or not to validate the response coming back from the API.
   *
   * @var bool
   */
  protected $validate_response;

  /**
   * Holds the account and site name for Lift Profiles.
   * @var array
   */
  protected $lift_profiles_account_name;

  /**
   * The singleton instance.
   *
   * @var AcquiaLiftAPI
   */
  private static $instance;


  /**
   * Holds an array of existing test names.
   *
   * @var array
   */
  protected static $existing_tests;

  /**
   * An http client for making calls to Acquia Lift.
   *
   * @var AcquiaLiftDrupalHttpClientInterface
   */
  protected $httpClient;

  protected $logger = NULL;

  /**
   * Whether the passed in status code represents a successful response.
   *
   * @param $code
   *   The status code.
   * @return bool
   *   TRUE if the code represents a client error, FALSE otherwise.
   */
  public static function isSuccessful($code) {
    return $code >= 200 && $code < 300;
  }

  /**
   * Whether the passed in status code represents a client side error.
   *
   * @param $code
   *   The status code.
   * @return bool
   *   TRUE if the code represents a client error, FALSE otherwise.
   */
  public static function isClientError($code) {
    return $code >= 400 && $code < 500;
  }

  /**
   * Whether the passed in status code represents a server side error.
   *
   * @param $code
   *   The status code.
   * @return bool
   *   TRUE if the code represents a server error, FALSE otherwise.
   */
  public static function isServerError($code) {
    return $code >= 500 && $code < 600;
  }

  /**
   * Maps the passed in response code to an exception class to use.
   *
   * If the response code passed in constitutes a successful response,
   * NULL is returned. If it does not constitute a 400 or 500 error,
   * NULL is returned.
   *
   * @param $code
   *   The response code to map.
   * @return null|string
   *   The exception class to use or NULL.
   */
  protected static function mapBadResponseToExceptionClass($code) {
    if (self::isSuccessful($code)) {
      return NULL;
    }
    if (self::isClientError($code)) {
      switch ($code) {
        case 404:
          return 'AcquiaLiftNotFoundException';
        case 403:
          return 'AcquiaLiftForbiddenException';
        default:
          return 'AcquiaLiftClientErrorException';
      }
    }
    elseif (self::isServerError($code)) {
      return 'AcquiaLiftServerErrorException';
    }
    return NULL;
  }

  /**
   * Resets the singleton instance.
   *
   * Used in unit tests.
   */
  public static function reset() {
    self::$instance = NULL;
  }

  /**
   * Singleton factory method.
   *
   * @return AcquiaLiftAPI
   */
  public static function getInstance($account_info) {
    if (empty(self::$instance)) {
      if (drupal_valid_test_ua()) {
        $broken = variable_get('acquia_lift_web_test_broken_client', FALSE);
        self::setTestInstance($broken);
        return self::$instance;
      }
      if (empty($account_info['api_url']) || !valid_url($account_info['api_url'])) {
        throw new AcquiaLiftCredsException('Acquia Lift API URL is missing or not a valid URL.');
      }
      $api_url = $account_info['api_url'];
      $needs_scheme = strpos($api_url, '://') === FALSE;

      if ($needs_scheme) {
        global $is_https;
        // Use the same scheme for Acquia Lift as we are using here.
        $url_scheme = ($is_https) ? 'https://' : 'http://';
        $api_url = $url_scheme . $api_url;
      }
      if (substr($api_url, -1) === '/') {
        $api_url = substr($api_url, 0, -1);
      }
      $acquia_lift_version = "undefined";

      if (!empty($account_info['public_key'])) {
        $public_key = $account_info['public_key'];
      }
      else {
        throw new AcquiaLiftCredsException('Acquia Lift Public Key was not found.');
      }
      if (!empty($account_info['private_key'])) {
        $private_key = $account_info['private_key'];
        // Additionally, decode the private key. According to the HMAC Spec 2.0 it is
        // transferred as base64 decoded string but we need to make sure we decode it before
        // using it.
        $private_key = base64_decode($private_key);
        // Validate if the secret key is a valid base64 encoded key.
        if ($private_key === FALSE) {
          throw new AcquiaLiftCredsException(t('secret key could not be decoded as base64. Please validate your credentials or contact Acquia Support.'));
        }
        // Do not continue if the decoded version is empty. Very unlikely that this will ever occur.
        if (empty($private_key)) {
          throw new AcquiaLiftCredsException(t('secret key cannot be empty. Please validate your credentials or contact Acquia Support.'));
        }
      }
      else {
        throw new AcquiaLiftCredsException('Acquia Lift Private Key was not found.');
      }
      if (!empty($account_info['acquia_lift_version'])) {
        $acquia_lift_version = $account_info['acquia_lift_version'];
      }
      $validate_response = $account_info['validate_response'];
      $lift_profiles_account_name = isset($account_info['profiles']['account_name']) ? $account_info['profiles']['account_name'] : '';
      // @todo Add integration with Acquia Customer Auth here. Can only happen
      // after Acquia Decision Service has integrated with Acquia Customer Auth and
      // customer auth has become public. This will add additional security as
      // we then can self-generate public and private keys per customer. They
      // will also be able to expire easily.
      self::$instance = new self($api_url, $public_key, $private_key, $acquia_lift_version, $validate_response, $lift_profiles_account_name);
    }
    return self::$instance;
  }

  /**
   * Returns an AcquiaLiftAPI instance with dummy creds and a dummy HttpClient.
   *
   * This is used during simpletest web tests.
   */
  public static function setTestInstance($broken_http_client = FALSE, $simulate_client_side_breakage = FALSE) {
    module_load_include('inc', 'acquia_lift', 'tests/acquia_lift.test_classes');
    self::$instance = new self('http://api.example.com', 'test-api-key', 'test-private-key', '7.x', FALSE, 'my-lift-web-account');
    // This method is only ever called within the context of a simpletest web
    // test, so the call to variable_get() is ok here.
    $test_data = variable_get('acquia_lift_web_test_data', array());
    self::$instance->setHttpClient(new DummyAcquiaLiftHttpClient($broken_http_client, $test_data, $simulate_client_side_breakage));
    self::$instance->setLogger(new AcquiaLiftTestLogger(FALSE));
  }

  /**
   * Private constructor as this is a singleton.
   *
   * @param string $api_url
   *   A string representing an Acquia Lift API url.
   */
  private function __construct($api_url, $public_key, $private_key, $acquia_lift_version, $validate_response = TRUE, $profiles_account_name = '') {
    $this->api_url = $api_url;
    $this->api_public_key = $public_key;
    $this->api_private_key = $private_key;
    $this->lift_version = $acquia_lift_version;
    $this->validate_response = $validate_response;
    $this->lift_profiles_account_name = $profiles_account_name;
  }

  /**
   * Returns an http client to use for Acquia Lift calls.
   *
   * @return AcquiaLiftDrupalHttpClientInterface
   */
  protected function httpClient() {
    if (!isset($this->httpClient)) {
      $this->httpClient = new AcquiaLiftDrupalHttpClient();
    }
    return $this->httpClient;
  }

  /**
   * Setter for the httpClient property.
   *
   * @param AcquiaLiftDrupalHttpClientInterface $client
   *   The http client to use.
   */
  public function setHttpClient(AcquiaLiftDrupalHttpClientInterface $client) {
    $this->httpClient = $client;
  }

  /**
   * Accessor for the api_url property.
   *
   * @return string
   */
  public function getApiUrl() {
    return $this->api_url;
  }

  public function getPublicKey() {
    return $this->api_public_key;
  }

  /**
   * Retrieves a list of existing campaigns..
   */
  public function getExistingAgentNames() {
    if (!empty(self::$existing_tests)) {
      return self::$existing_tests;
    }
    $campaigns = $this->getCampaigns();
    $names = array();
    foreach ($campaigns as $campaign) {
      $names[] = $campaign['id'];
    }
    self::$existing_tests = $names;
    return $names;
  }

  public function getCampaigns() {
    $url = $this->generateEndpoint("campaigns");
    $fail_msg = 'Could not retrieve agents from Lift';
    $response = $this->makeGetRequest($url, array(), $fail_msg);
    if (empty($response) || isset($response['error'])) {
      return array();
    }
    return $response;
  }

  /**
   * Returns the campaign with the provided name if it exists, FALSE otherwise.
   */
  public function getAgent($agent_name) {
    $url = $this->generateEndpoint("campaigns/$agent_name");
    $fail_msg = 'Could not retrieve the specified agent from Lift';
    $response = $this->makeGetRequest($url, array(), $fail_msg);
    if (!empty($response)) {
      return $response;
    }
    return FALSE;
  }

  /**
   * Deletes the campaign with the specified name.
   */
  public function deleteAgent($agent_name) {
    $url = $this->generateEndpoint("campaigns/$agent_name");
    $vars = array('agent' => $agent_name);
    $success_msg = "The personalization {agent} has been deleted from Acquia Lift";
    $fail_msg = "The personalization {agent} could not be deleted from Acquia Lift";
    $this->makeDeleteRequest($url, array(), $success_msg, $fail_msg, $vars);
  }

  /**
   * Retieves the goals defined for hte specified campaign.
   */
  public function getGoalsForAgent($agent_name) {
    return array();
  }

  /**
   * Executes a ping request
   *
   * @return bool
   *   TRUE if the connection succeeded, FALSE otherwise.
   */
  public function ping() {
    $url = $this->generateEndpoint("ping");
    $fail_msg = 'Acquia Lift Testing Service could not be reached';
    try {
      $response = $this->makeGetRequest($url, array(), $fail_msg);
      if (!empty($response)) {
        return TRUE;
      }
    }
    catch(Exception $e) {
      return FALSE;
    }
    return FALSE;
  }

  /**
   * Saves a campaign to Lift.
   *
   * @param $name
   *   The name of the campaign. Should conform to Lift naming rules.
   * @param $title
   *   A friendly name for the campaign.
   * @param $decision_name
   *   The name of the decision for this campaign.
   * @param $goals
   *   An array of goals for this campaign.
   * @param $mab
   *   Whether to use the MAB algorithm.
   * @param $control
   *   The control fraction to use.
   * @param $explore
   *   The explore fraction to use.
   */
  public function saveCampaign($name, $title, $decision_name, $goals, $mab = FALSE, $control = 0, $explore = 0.2) {
    $vars = array('agent' => $name);
    $success_msg = 'The personalization {agent} was pushed to Acquia Lift';
    $fail_msg = 'The personalization {agent} could not be pushed to Acquia Lift';
    $path = "campaigns";
    $url = $this->generateEndpoint($path);
    $agent = array(
      'id' => $name,
      'title' => $title,
      'algorithm' => 'mab',
      'decision_sets' => array($decision_name),
      'goals' => $goals,
      // this manages the traffic fraction. We want to see the control variation
      // in 10% (configurable) of the times so the other 90% are the ones who
      // participate. This means we have to send 1-0.1 = 0.9 to the traffic
      // fraction variable
      'traffic_fraction'  =>  1-$control,
      // Non-MAB is equivalent to 100% explore mode.
      'explore_fraction' => $mab ? $explore : 1,
    );
    $this->makePostRequest($url, array(), $agent, $success_msg, $fail_msg, $vars);
  }

  /**
   * Saves a decision set to Lift.
   *
   * @param $name
   *   The machine name of the decision set.
   * @param $title
   *   The human-readable name of the decision set.
   * @param $options
   *   An array of choices to be decided between.
   */
  public function saveDecisionSet($name, $title, $options) {
    $decision_set = array(
      'id' => $name,
      'title' => $title,
      'decisions' => array()
    );
    foreach ($options as $option) {
      $decision_set['decisions'][] = array(
        'external_id' => $option['option_id']
      );
    }
    $path = "decision_sets";
    $url = $this->generateEndpoint($path);
    $vars = array('name' => $title);
    $success_msg = 'The Decision Set {name} was pushed to Acquia Lift';
    $fail_msg = 'The Decision Set {name} could not be pushed to Acquia Lift';
    $this->makePostRequest($url, $this->getPutHeaders(), $decision_set, $success_msg, $fail_msg, $vars);
  }

  /**
   * Saves a goal to Lift.
   *
   * @param $name
   *   The machine name of the goal.
   */
  public function saveGoal($name) {
    $goal = array(
      'id' => $name,
      'title' => $name,
      'description' => $name
    );
    $path = "goals";
    $url = $this->generateEndpoint($path);
    $vars = array('name' => $name);
    $success_msg = 'The Goal {name} was pushed to Acquia Lift';
    $fail_msg = 'The Goal {name} could not be pushed to Acquia Lift';
    $this->makePostRequest($url, $this->getPutHeaders(), $goal, $success_msg, $fail_msg, $vars);
  }

  /**
   * Implements AcquiaLiftLearnReportDataSourceInterface::getReportForDateRange().
   */
  public function getReportForDateRange($name, $from, $to) {
    if ($from != $to && ($interval = date_diff(date_create($from), date_create($to)))) {
      $days = $interval->format('%d');
    }
    else {
      $days = 1;
    }
    $date = new DateTime($from, new DateTimeZone("UTC"));
    $from = $date->format("Y-m-d\TH:i:s\Z");
    $path = "report?campaign_id=$name&from=$from&days=$days";
    $url = $this->generateEndpoint($path);
    $vars = array('name' => $name);
    $fail_msg = 'Could not retrieve report from Acquia Lift';
    return $this->makeGetRequest($url, $this->getStandardHeaders(), $fail_msg, $vars);
  }

  public function getPersonalizationOverviewReport($name, $variations) {
    if (empty($this->lift_profiles_account_name)) {
      throw new AcquiaLiftCredsException('No account name for Lift Profiles configured');
    }
    $site_name = variable_get('acquia_lift_profiles_site_name', '');
    if (empty($site_name)) {
      $site_name = 'Drupal';
    }
    $path = "report/overview?account={$this->lift_profiles_account_name}&site=$site_name&personalization=$name";
    foreach ($variations as $variation) {
      $path .= "&variation=$variation";
    }
    $url = $this->generateEndpoint($path);
    $vars = array('name' => $name);
    $fail_msg = 'Could not retrieve report from Acquia Lift';
    return $this->makeGetRequest($url, $this->getStandardHeaders(), $fail_msg, $vars);
  }

  public function getAudienceReport($name, $audience, $variations, $type, $from, $to, $goal = NULL) {
    if ($type == 'target' && count($variations) > 1) {
      throw new AcquiaLiftException("Can't retrieve targeting report for more than one variation");
    }
    if (empty($this->lift_profiles_account_name)) {
      throw new AcquiaLiftCredsException('No account name for Lift Profiles configured');
    }
    $site_name = variable_get('acquia_lift_profiles_site_name', '');
    if (empty($site_name)) {
      $site_name = 'Drupal';
    }
    $tz = drupal_get_user_timezone();
    $from_datetime = date_create("@$from", new DateTimeZone($tz));
    $to_datetime = date_create("@$to", new DateTimeZone($tz));
    $from_str = $from_datetime->format('Y-m-d');
    $to_str = $to_datetime->format('Y-m-d');
    $path = "report/$type?account={$this->lift_profiles_account_name}&site=$site_name&personalization=$name&audience=$audience&from=$from_str&to=$to_str";
    foreach ($variations as $variation) {
      $path .= "&variation=$variation";
    }
    if (!empty($goal)) {
      $path .= "&goal=$goal";
    }
    $url = $this->generateEndpoint($path);
    $vars = array('name' => $name);
    $fail_msg = 'Could not retrieve report from Acquia Lift';
    return $this->makeGetRequest($url, $this->getStandardHeaders(), $fail_msg, $vars);
  }

  /**
   * Returns the fully qualified URL to use to connect to API.
   *
   * This function handles personalizing the endpoint to the client by
   * handling owner code and API keys.
   *
   * @param $path
   *   The $path to the endpoint at the API base url.
   * @param $admin
   *   Boolean indicating whether to use admin key (true) or runtime (false).
   */
  protected function generateEndpoint($path, $admin = TRUE) {
    $endpoint = $this->api_url;
    if (substr($path, 0, 1) !== '/') {
      $endpoint .= '/';
    }
    $endpoint .= $path;
    return $endpoint;
  }

  /**
   * Returns the canonical representation of a request.
   *
   * @param $verb
   *  The request method, e.g. 'GET'.
   * @param $host
   *  The host to send our request to. Eg domain1.com
   * @param $path
   *  The path of the request, e.g. 'play'.
   * @param string $query_args
   *  Array of queries that are sent along
   * @param array $auth_options
   * @param $timestamp
   * @param $content_type
   * @param $payload_hash
   * @return string
   *   The canonical representation of the request.
   */
  function canonicalizeRequest($verb, $host, $path, $query_args = "", $auth_options = array(), $timestamp, $content_type, $payload_hash) {
    // See https://github.com/acquia/http-hmac-spec (branch 2.0) for
    // specification on Canonicalization of the request
    $list = array();
    // The uppercase HTTP request method e.g. "GET", "POST"
    $list[] = strtoupper($verb);
    // The (lowercase) hostname, matching the HTTP "Host" request header field
    // (including any port number)
    $list[] = $host;
    // The HTTP request path with leading slash, e.g. /resource/11
    $list[] = $path;
    // Any query parameters or empty string. This should be the exact string sent
    // by the client, including urlencoding.
    $list[] = $query_args;
    // normalized parameters similar to section 9.1.1 of OAuth 1.0a. The
    // parameters are the id, nonce, realm, and version from the Authorization
    // header. Parameters are sorted by name and separated by '&' with name and
    // value separated by =, percent encoded (urlencoded).
    ksort($auth_options);
    $auth_options_list = array();
    foreach ($auth_options as $id => $value) {
      $auth_options_list[] = $id . "=" . $value;
    }
    $list[] = implode($auth_options_list, "&");
    // The normalized header names and values specified in the headers parameter
    // of the Authorization header. Names should be lower-cased, sorted by name,
    // separated from value by a colon and the value followed by a newline so
    // each extra header is on its own line.

    // @todo Additional headers are not supported for now.

    // The value of the X-Authorization-Timestamp header
    $list[] = $timestamp;
    // Some values such as Content Type and the BodyHash should be ommitted if
    // the Content-Length = 0
    if (!empty($payload_hash)) {
      // Content Type.
      $list[] = strtolower($content_type);
      // SHA256 of the Request-Body.
      $list[] = $payload_hash;
    }
    return implode("\n", $list);
  }

  /**
   * Returns a string to use for the 'Authorization' header.

   * @param string $method
   *   The method used for the request such as PUT, POST, ...
   * @param string $path
   *   The full path of the request.
   * @param array $headers
   *   An array of headers that will be used to generate the authentication
   *   header.
   * @param null|array $payload
   *   The post body raw data, usually a json string but could also be XML.
   * @return string
   *   The value of the Authorization header.
   * @throws Exception
   *   Throws an Exception if it was unable to decode the secret key.
   */
  public function getAuthHeader($verb, $url, $headers = array(), $payload_hash = "", $nonce = "") {
    // Generate Authorization array
    $authorization = array();
    $authorization["realm"] = "dice";
    $authorization["id"] = $this->api_public_key;
    $authorization["nonce"] = $nonce;
    $authorization["version"] = "2.0";

    // Get Host
    $parsed_url = parse_url($url);
    $host = $parsed_url['host'];
    if (!empty($parsed_url['port'])) {
      $host .= ":" . $parsed_url['port'];
    }

    // Get Path
    $path = $parsed_url['path'];

    // Query Param
    $query_args = "";
    if (isset($parsed_url['query'])) {
      $query_args = $parsed_url['query'];
    }

    // Time
    $timestamp = $headers['X-Authorization-Timestamp'];

    // Content Type
    $content_type = $headers['Content-Type'];

    // Canonicalize the request
    $message = $this->canonicalizeRequest($verb, $host, $path, $query_args, $authorization, $timestamp, $content_type, $payload_hash);

    // Create an HMAC hash with the base64 decoded secret key and make sure we always use binary output.
    // The message is converted into a string so that we do not encode a boolean or any other type than string
    // since the server side also expects the message to always be a string.
    $signature = base64_encode(hash_hmac('sha256', (string) $message, $this->api_private_key, TRUE));
    $authorization["signature"] = $signature;
    $auth_stringlist = array();
    foreach ($authorization as $id => $value) {
      $auth_stringlist[] = $id . "=" . "\"" . $value . "\"";
    }
    // Set the Authorization header
    return "acquia-http-hmac" . " " . implode($auth_stringlist, ",");
  }

  /**
   * @param string $nonce
   *   Nonce used in the request
   * @param string $timestamp
   *   Timestamp used in the request
   * @param string $body
   *   Body from the response
   * @return string
   *   Base64 of the hash of the canonicalized version of the response
   */
  public function getResponseSignature($nonce, $timestamp, $body = "") {
    // See https://github.com/acquia/http-hmac-spec (branch 2.0) for
    //specification on canonicalization of the response
    $list = array();

    // Nonce + "\n" +
    $list[] = $nonce;

    // Timestamp
    $list[] = $timestamp;

    // Body
    $list[] = $body;
    $message =  implode("\n", $list);
    // Always use binary output
    return base64_encode(hash_hmac('sha256', (string) $message, $this->api_private_key, TRUE));
  }

  /**
   * Add necessary headers before sending it to our Acquia Service
   *
   * @param string $method
   *   The method used for the request such as PUT, POST, ...
   * @param string $path
   *   The full path of the request
   * @param array $headers
   *   An array of headers that will be used to generate the authentication
   *   header.
   * @param string|array $payload
   *   The full request payload that will be sent to the server. If it is
   *   an array, it will be encoded to json first
   * @throws Exception
   *   Throws an Exception if it was unable to add the Authorization header.
   */
  protected function prepareRequest($method, &$url, &$headers, $payload = "", $nonce = "", $timestamp) {
    // Add tracking information and our client information
    $id = uniqid();
    if (!stristr($url,'?')) {
      $url .= "?";
    }
    else {
      $url .= "&";
    }
    $url .= 'request_id=' . $id;
    $url .= '&client_id=' . $this->api_public_key;

    // Add current Lift Drupal version
    $headers += array('User-Agent' => 'acquia_lift/'. $this->lift_version);
    $headers += array('Content-Type' => 'application/json');
    $headers += array('X-Authorization-Timestamp' => $timestamp);

    // SHA256 of Payload, binary form
    $payload_hash = "";
    // Set it to the headers if we have payload information where acceptable.
    if ($method != "GET" || $method != "HEAD") {
      if (isset($payload) && !empty($payload)) {
        $payload_hash = base64_encode(hash("sha256", $payload, TRUE));
        $headers['X-Authorization-Content-SHA256'] = $payload_hash;
      }
    }

    // Calculate the Authorization headers and set it if we succeed. Throw an
    // Exception if it is not able to calculate a hash.
    $auth_header = $this->getAuthHeader($method, $url, $headers, $payload_hash, $nonce);
    if (empty($auth_header)) {
      throw new AcquiaLiftCredsException(t('Invalid authentication string - subscription keys expired or missing.'));
    }
    $headers += array('Authorization' => $auth_header);
  }

  /**
   * Generate a nonce string
   *
   * @return string
   *   the generated nonce
   */
  protected static function nonce() {
    return sprintf('%04x%04x-%04x-%04x-%04x-%04x%04x%04x',

      // 32 bits for "time_low"
      mt_rand(0, 0xffff), mt_rand(0, 0xffff),

      // 16 bits for "time_mid"
      mt_rand(0, 0xffff),

      // 16 bits for "time_hi_and_version",
      // four most significant bits holds version number 4
      mt_rand(0, 0x0fff) | 0x4000,

      // 16 bits, 8 bits for "clk_seq_hi_res",
      // 8 bits for "clk_seq_low",
      // two most significant bits holds zero and one for variant DCE1.1
      mt_rand(0, 0x3fff) | 0x8000,

      // 48 bits for "node"
      mt_rand(0, 0xffff), mt_rand(0, 0xffff), mt_rand(0, 0xffff)
    );

  }

  /**
   * @param string $nonce
   *   The nonce used in the request
   * @param string $timestamp
   *   The timestamp used in the request
   * @param object $response
   *   The full response that returned from the Acquia Service.
   * @throws Exception?
   *   Throws exceptions if the response did not match with the credentials of
   *   the current website.
   */
  protected function authenticateResponse($nonce, $timestamp, $response) {
    if ($this->validate_response) {
      // Fail when there is no Authorization header.
      if (!isset($response->headers['x-server-authorization-hmac-sha256'])) {
        throw new Exception('Authentication of Acquia Lift Response not found for path');
      }
      // Fail when the hash does not match.
      $response_hmac = $this->getResponseSignature($nonce, $timestamp, $response->data);
      if ($response_hmac !== $response->headers['x-server-authorization-hmac-sha256']) {
        throw new Exception('Authentication of Acquia Lift Response failed');
      }
    }
  }

  /**
   * Makes an Acquia Authenticated GET request to an API endpoint.
   *
   * @param $url
   *   The endpoint path.
   * @param $headers
   *   Any headers that need to be added.
   * @param $fail_msg
   *   The error message to provide if the request fails.
   * @return array
   *   The decoded response as an array.
   * @throws \AcquiaLiftException
   */
  protected function makeGetRequest($url, $headers, $fail_msg, $vars = array()) {
    // Create unique nonce for this request
    $nonce = self::nonce();
    $timestamp = time();
    $this->prepareRequest('GET', $url, $headers, "", $nonce, $timestamp);
    $response = $this->httpClient()->get($url, $headers);
    if ($response->code != 200) {
      $this->handleBadResponse($response->code, $fail_msg);
      return array();
    }
    $this->authenticateResponse($nonce, $timestamp, $response);
    return json_decode($response->data, TRUE);
  }

  /**
   * Makes an Acquia Authenticated PUT request to an API endpoint.
   *
   * @param string $url
   *   The fully-qualified URL to make the request to.
   * @param array $headers
   *   Any headers that need to be added.
   * @param array $body
   *   The request body.
   * @param string $success_msg
   *   The error message to provide if the request fails.
   * @param string $fail_msg
   *   The error message to provide if the request fails.
   * @throws \AcquiaLiftException
   */
  protected function makePutRequest($url, $headers = array(), $body = NULL, $success_msg = '', $fail_msg = '', $vars = array()) {
    // Our authentication needs the JSON string before our HTTP converts the
    // array of items. To avoid double work, we encode it here and send it along to both.
    $payload = $this->encodeBody($body);
    $nonce = self::nonce();
    $timestamp = time();
    $this->prepareRequest('PUT', $url, $headers, $payload, $nonce, $timestamp);
    $response = $this->httpClient()->put($url, $headers, $payload);
    if ($response->code == 200) {
      $this->logger()->log(PersonalizeLogLevel::INFO, $success_msg, $vars);
    }
    else {
      $this->handleBadResponse($response->code, $fail_msg, $vars);
    }

    $this->authenticateResponse($nonce, $timestamp, $response);
    return json_decode($response->data, TRUE);
  }

  /**
   * Makes an Acquia Authenticated POST request to an API endpoint.
   *
   * @param string $url
   *   The fully-qualified URL to make the request to.
   * @param array $headers
   *   Any headers that need to be added.
   * @param array $body
   *   The request body.
   * @param string $success_msg
   *   The error message to provide if the request fails.
   * @param string $fail_msg
   *   The error message to provide if the request fails.
   * @throws \AcquiaLiftException
   */
  protected function makePostRequest($url, $headers = array(), $body = NULL, $success_msg = '', $fail_msg = '', $vars = array()) {
    $payload = $this->encodeBody($body);
    $nonce = self::nonce();
    $timestamp = time();
    $this->prepareRequest('POST', $url, $headers, $payload, $nonce, $timestamp);
    $response = $this->httpClient()->post($url, $headers, $payload);
    if ($response->code == 200) {
      $this->logger()->log(PersonalizeLogLevel::INFO, $success_msg, $vars);
    }
    else {
      $this->handleBadResponse($response->code, $fail_msg, $vars);
    }
    $this->authenticateResponse($nonce, $timestamp, $response);
    return json_decode($response->data, TRUE);
  }

  /**
   * Makes an Acquia Authenticated DELETE request to an API endpoint.
   *
   * @param $path
   *   The endpoint path.
   * @param $headers
   *   Any headers that need to be added.
   * @param $success_msg
   *   The error message to provide if the request fails.
   * @param $fail_msg
   *   The error message to provide if the request fails.
   * @throws \AcquiaLiftException
   */
  protected function makeDeleteRequest($url, $headers = array(), $success_msg, $fail_msg, $vars) {
    $nonce = self::nonce();
    $timestamp = time();
    $this->prepareRequest('DELETE', $url, $headers, "", $nonce, $timestamp);
    $response = $this->httpClient()->delete($url, $headers);
    if ($response->code == 200) {
      $this->logger()->log(PersonalizeLogLevel::INFO, $success_msg, $vars);
    }
    else {
      // Delete requests should fail softly, i.e. just with logging, not throwing
      // an exception.
      $this->handleBadResponse($response->code, $fail_msg, $vars, TRUE, TRUE);
    }
    $this->authenticateResponse($nonce, $timestamp, $response);
    return json_decode($response->data, TRUE);
  }

  /**
   * Encode request payload.
   *
   * If it is an array, encode it to json, otherwise keep it as is.
   *
   * @param $body
   * @return string
   *
   */
  protected function encodeBody($body) {
    if (is_string($body)) {
      $data = $body;
    }
    else {
      $data = drupal_json_encode($body);
    }
    return $data;
  }

  /**
   * Returns an array of headers to use for API requests.
   */
  protected function getStandardHeaders() {
    return array('Accept' => 'application/json');
  }

  /**
   * Returns an array of headers to use for API PUT requests.
   */
  protected function getPutHeaders() {
    return array(
      'Content-Type' => 'application/json; charset=utf-8',
      'Accept' => 'application/json'
    );
  }

  /**
   * Returns the logger to use.
   *
   * @return PersonalizeLoggerInterface
   */
  protected function logger() {
    if ($this->logger !== NULL) {
      return $this->logger;
    }
    return new PersonalizeLogger();
  }

  /**
   * Implements PersonalizeLoggerAwareInterface::setLogger().
   */
  public function setLogger(PersonalizeLoggerInterface $logger) {
    $this->logger = $logger;
  }

  /**
   * Figures out the correct exception to throw and throws it.
   *
   * @param $response_code
   *   The response code, e.g. 404.
   * @param $fail_msg
   *   The message to use for the exception, and for logging if $log_failure it TRUE.
   * @param array $vars
   *   Variables to pass to the message.
   * @param bool $log_failure
   *   Whether failures should be logged or not
   * @param bool $soft_fail
   *   Whether to fail softly, i.e. without throwing an exception.
   */
  public function handleBadResponse($response_code, $fail_msg, $vars = array(), $log_failure = TRUE, $soft_fail = FALSE) {
    if ($exception_class = self::mapBadResponseToExceptionClass($response_code)) {
      if ($log_failure) {
        $logger = $this->logger();
        $logger->log(PersonalizeLogLevel::ERROR, $fail_msg, $vars);
      }
      if (!$soft_fail) {
        throw new $exception_class(t($fail_msg, $vars));
      }
    }
    if (self::isSuccessful($response_code)) {
      // If the response wasn't actually bad but we still got here somehow, just
      // log a warning and return.
      if ($log_failure) {
        $this->logger()->log(PersonalizeLogLevel::WARNING, $fail_msg, $vars);
      }
      return;
    }
    if (!$soft_fail) {
      // Otherwise throw a generic exception.
      throw new AcquiaLiftException(t($fail_msg, $vars));
    }
  }

  /**
   * Returns a unique agent name based on the name passed in.
   *
   * Checks existing agents in Acquia Lift and adds a suffix if the
   * passed in name already exists. Also ensures the name is within
   * the smaller of Acquia Lift's max length restriction and the
   * passed in max length restriction.
   *
   * @param $agent_name
   *   The desired agent name.
   *
   * @param $max_length
   *   The max length restriction other than that imposed by Acquia
   *   Lift itself. The function will use the smaller of the two
   *   max length restrictions.

   * @return string
   *   A machine-readable name for the agent that does not exist yet
   *   in Acquia Lift.
   */
  public function ensureUniqueAgentName($agent_name, $max_length) {
    if ($max_length > self::NAME_MAX_LENGTH) {
      $max_length = self::NAME_MAX_LENGTH;
    }
    $agent_name = substr($agent_name, 0, $max_length);

    $existing = $this->getExistingAgentNames();
    $index = 0;
    $suffix = '';
    while(in_array($agent_name . $suffix, $existing)) {
      $suffix = '-' . $index;
      while (strlen($agent_name . $suffix) > $max_length) {
        $agent_name = substr($agent_name, 0, -1);
      }
      $index++;
    }
    $new_name = $agent_name . $suffix;
    self::$existing_tests[] = $new_name;
    return $new_name;
  }
}
