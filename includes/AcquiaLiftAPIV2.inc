<?php

class AcquiaLiftAPIV2 extends AcquiaLiftAPIBase implements AcquiaLiftLearnReportDataSourceInterface {

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
   * The singleton instance.
   *
   * @var AcquiaLiftAPI
   */
  private static $instance;

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
      }
      else {
        throw new AcquiaLiftCredsException('Acquia Lift Private Key was not found.');
      }
      if (!empty($account_info['acquia_lift_version'])) {
        $acquia_lift_version = $account_info['acquia_lift_version'];
      }
      $validate_response = $account_info['validate_response'];
      // @todo Add integration with Acquia Customer Auth here. Can only happen
      // after Acquia Decision Service has integrated with Acquia Customer Auth and
      // customer auth has become public. This will add additional security as
      // we then can self-generate public and private keys per customer. They
      // will also be able to expire easily.
      self::$instance = new self($api_url, $public_key, $private_key, $acquia_lift_version, $validate_response);
    }
    return self::$instance;
  }

  /**
   * Returns an AcquiaLiftAPIV2 instance with dummy creds and a dummy HttpClient.
   *
   * This is used during simpletest web tests.
   */
  public static function setTestInstance($broken_http_client = FALSE, $simulate_client_side_breakage = FALSE) {
    module_load_include('inc', 'acquia_lift', 'tests/acquia_lift.test_classes');
    self::$instance = new self('http://api.example.com', 'test-api-key', 'test-private-key', '7.x', FALSE);
    // This method is only ever called within the context of a simpletest web
    // test, so the call to variable_get() is ok here.
    $test_data = variable_get('acquia_lift_web_test_data', array());
    self::$instance->setHttpClient(new DummyAcquiaLiftHttpClient(ACQUIA_LIFT_TESTING_AGENT_V2, $broken_http_client, $test_data, $simulate_client_side_breakage));
    self::$instance->setLogger(new AcquiaLiftTestLogger(FALSE));
  }

  /**
   * Private constructor as this is a singleton.
   *
   * @param string $api_url
   *   A string representing an Acquia Lift API url.
   */
  private function __construct($api_url, $public_key, $private_key, $acquia_lift_version, $validate_response = TRUE) {
    $this->api_url = $api_url;
    $this->api_public_key = $public_key;
    $this->api_private_key = $private_key;
    $this->lift_version = $acquia_lift_version;
    $this->validate_response = $validate_response;
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
   * Implements AcquiaLiftAPIInterface::getExistingAgentNames().
   */
  public function getExistingAgentNames() {
    $campaigns = $this->getCampaigns();
    $names = array();
    foreach ($campaigns as $campaign) {
      $names[] = $campaign['id'];
    }
    return $names;
  }

  public function getCampaigns() {
    $url = $this->generateEndpoint("campaigns");
    $fail_msg = 'Could not retrieve agents from Lift';
    $response = $this->makeGetRequest($url, array(), $fail_msg);
    if (!empty($response)) {
      return $response;
    }
    return array();
  }

  /**
   * Implements AcquiaLiftAPIInterface::getAgent();
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
   * Implements AcquiaLiftAPIInterface::deleteAgent();
   */
  public function deleteAgent($agent_name) {
    $url = $this->generateEndpoint("campaigns/$agent_name");
    $vars = array('agent' => $agent_name);
    $success_msg = "The personalization {agent} has been deleted from Acquia Lift";
    $fail_msg = "The personalization {agent} could not be deleted from Acquia Lift";
    $this->makeDeleteRequest($url, array(), $success_msg, $fail_msg, $vars);
  }

  /**
   * Implements AcquiaLiftAPIInterface::getGoalsForAgent();
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
    // We use the list-agents endpoint for our ping test, in the absence of
    // an endpoint specifically provided for this purpose.
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
    if ($from == $to) {
      $days = 1;
    }
    else {
      $interval = date_diff(date_create($from), date_create($to));
      $days = $interval->format('%d');
    }
    $date = new DateTime($from, new DateTimeZone("UTC"));
    $from = $date->format("Y-m-d\TH:i:s\Z");
    $path = "report?campaign_id=$name&from=$from&days=$days";
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
    //specification on Canonicalization of the request
    $list = array();

    // HTTP-Verb + "\n" +
    $list[] = strtoupper($verb);

    // Host
    $list[] = $host;

    // Path
    $list[] = $path;

    // Query Params
    $list[] = $query_args;

    // Authorization Params such as Nonce
    ksort($auth_options);
    $auth_options_list = array();
    foreach ($auth_options as $id => $value) {
      $auth_options_list[] = $id . "=" . $value;
    }
    $list[] = implode($auth_options_list, "&");

    // Added Signed Headers.
    // @todo Implement this when needed.
    $list[] = "";

    // Timestamp
    $list[] = $timestamp;

    // Content Type
    $list[] = strtolower($content_type);

    // Body Hash
    $list[] = $payload_hash;

    return implode("\n", $list);
  }

  /**
   *
   * @return string
   */

  /**
   * Returns a string to use for the 'Authorization' header.

   * @param string $method
   *   The method used for the request such as PUT, POST, ...
   * @param string $path
   *   The full path of the request
   * @param array $headers
   *   An array of headers that will be used to generate the authentication
   *   header.
   * @param null|array $payload
   *   The post body raw data, usually a json string but could also be XML.
   * @return string
   *   The value of the Authorization header.
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

    // Always use binary output
    $signature = base64_encode(hash_hmac('sha256', $this->api_private_key, (string) $message, TRUE));
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
    return base64_encode(hash_hmac('sha256', $this->api_private_key, (string) $message, TRUE));
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
    $response = parent::makeGetRequest($url, $headers, $fail_msg);
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
    $response = parent::makePutRequest($url, $headers, $payload, $success_msg, $fail_msg, $vars);
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
    $response = parent::makePostRequest($url, $headers, $payload, $success_msg, $fail_msg, $vars);
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
    $response = parent::makeDeleteRequest($url, $headers, $success_msg, $fail_msg, $vars);
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

}
