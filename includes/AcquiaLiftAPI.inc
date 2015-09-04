<?php


class AcquiaLiftAPI extends AcquiaLiftAPIBase implements AcquiaLiftReportDataSourceInterface {

  const API_URL = 'api.lift.acquia.com';

  const EXPLORATION_RATE_RANDOM = 1;

  const STRING_REPLACE_PATTERN = '[^A-Za-z0-9_-]';

  const VALID_NAME_MATCH_PATTERN = '[A-Za-z0-9_-]';

  const ENABLED_STATUS = 'enabled';

  const PAUSED_STATUS = 'paused';

  const STOPPED_STATUS = 'stopped';

  const PROVISIONAL_STATUS = 'provisional';

  /**
   * The value representing no features applied to an experiment.
   */
  const NO_FEATURES = '(none)';

  /**
   * The Acquia Lift API key to use.
   *
   * @var string
   */
  protected $api_key;

  /**
   * The Acquia Lift admin key to use.
   *
   * @var string
   */
  protected $admin_key;

  /**
   * The Acquia Lift owner code to use.
   *
   * @var string
   */
  protected $owner_code;

  /**
   * The Acquia Lift API url to use.
   *
   * @var string
   */
  protected $api_url;

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
      foreach (array('api_key', 'admin_key', 'owner_code') as $key) {
        if (!isset($account_info[$key])) {
          throw new AcquiaLiftCredsException('Acquia Lift account info is not complete.');
        }
      }
      if (!self::codeIsValid($account_info['owner_code'])) {
        throw new AcquiaLiftCredsException('Acquia Lift owner code is invalid.');
      }

      $api_url = self::API_URL;
      $needs_scheme = TRUE;
      if (!empty($account_info['api_url'])) {
        if (!valid_url($account_info['api_url'])) {
          throw new AcquiaLiftCredsException('Acquia Lift API URL is not a valid URL.');
        }
        $api_url = $account_info['api_url'];
        $needs_scheme = strpos($api_url, '://') === FALSE;
      }
      if ($needs_scheme) {
        global $is_https;
        // Use the same scheme for Acquia Lift as we are using here.
        $url_scheme = ($is_https) ? 'https://' : 'http://';
        $api_url = $url_scheme . $api_url;
      }
      if (substr($api_url, -1) === '/') {
        $api_url = substr($api_url, 0, -1);
      }

      self::$instance = new self($account_info['api_key'], $account_info['admin_key'], $account_info['owner_code'], $api_url);
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
    self::$instance = new self('test-api-key', 'test-admin-key', 'test-owner-code', 'http://api.example.com');
    // This method is only ever called within the context of a simpletest web
    // test, so the call to variable_get() is ok here.
    $test_data = variable_get('acquia_lift_web_test_data', array());
    self::$instance->setHttpClient(new DummyAcquiaLiftHttpClient(ACQUIA_LIFT_TESTING_AGENT_V1, $broken_http_client, $test_data, $simulate_client_side_breakage));
    self::$instance->setLogger(new AcquiaLiftTestLogger(FALSE));
  }

  /**
   * Private constructor as this is a singleton.
   *
   * @param string $api_key
   *   A string representing an Acquia Lift API key.
   * @param string $admin_key
   *   A string representing an Acquia Lift admin key.
   * @param string $owner_code
   *   A string representing an Acquia Lift owner code.
   * @param string $api_url
   *   A string representing an Acquia Lift API url.
   */
  private function __construct($api_key, $admin_key, $owner_code, $api_url) {
    $this->api_key = $api_key;
    $this->admin_key = $admin_key;
    $this->owner_code = $owner_code;
    $this->api_url = $api_url;
  }

  /**
   * Accessor for the api_key property.
   *
   * @return string
   */
  public function getApiKey() {
    return $this->api_key;
  }

  /**
   * Accessor for the admin_key property.
   *
   * @return string
   */
  public function getAdminKey() {
    return $this->admin_key;
  }

  /**
   * Accessor for the owner_code property.
   *
   * @return string
   */
  public function getOwnerCode() {
    return $this->owner_code;
  }

  /**
   * Accessor for the api_url property.
   *
   * @return string
   */
  public function getApiUrl() {
    return $this->api_url;
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
    $endpoint = $this->api_url . '/';
    $endpoint .= $this->owner_code;
    if (substr($path, 0, 1) !== '/') {
      $endpoint .= '/';
    }
    $endpoint .= $path;
    // Append api key.
    if (strpos($endpoint, '?')) {
      $endpoint .= '&';
    }
    else {
      $endpoint .= '?';
    }
    $key = $admin ? $this->getAdminKey() : $this->getApiKey();
    $endpoint .= "apikey=$key";
    return $endpoint;
  }

  /**
   * Allows legacy agents to make old API requests that are no longer explicitly
   * supported by this API client.
   *
   * @param $path
   *   The endpoint path, including any querystring parameters.
   * @param $headers
   *   Any headers that need to be added.
   * @param $error
   *   The error message to provide if the request fails.
   * @return array
   *   The decoded response as an array.
   * @throws \AcquiaLiftException
   */
  public function makeLegacyGetRequest($path, $headers, $error) {
    $headers += array('Accept' => 'application/json');
    $url = $this->generateEndpoint($path);
    $response = $this->httpClient()->get($url, $headers);
    if ($response->code != 200) {
      $this->handleBadResponse($response->code, $error);
      return array();
    }
    return json_decode($response->data, TRUE);
  }

  /**
   * Tests the connection to Acquia Lift.
   *
   * @return bool
   *   TRUE if the connection succeeded, FALSE otherwise.
   */
  public function ping() {
    // We use the list-agents endpoint for our ping test, in the absence of
    // an endpoint specifically provided for this purpose.
    $url = $this->generateEndpoint("/list-agents");
    $admin_response = $this->httpClient()->get($url, array('Accept' => 'application/json'));
    $url = $this->generateEndpoint('/ping-test-agent/expire', FALSE);
    $runtime_response = $this->httpClient()->post($url, array('Accept' => 'application/json'));
    return $admin_response->code == 200 && $runtime_response->code != 403;
  }

  /**
   * Saves an agent to Acquia Lift.
   *
   * @param $machine_name
   *   The machine of the agent.
   * @param $label
   *   The human-readable name of the agent.
   * @param $decision_style
   *   The decision style to use, either 'random' or 'adaptive'
   * @param $status
   *   The status to set the agent to.
   * @param $control_rate
   *   A number between 0 and 1 inclusive representing the percentage to use as a
   *   control group.
   * @param $explore_rate
   *   A number between 0 and 1 inclusive representing the percentage to use as
   *   the continuous experiment group.
   * @param $sticky
   *   Boolean indicating whether decisions made by the agent should stick for each
   *   visitor.
   * @return boolean
   *   TRUE if the agent has been saved to Acquia Lift, FALSE otherwise.
   */
  public function saveAgent($machine_name, $label, $decision_style = 'random', $status = 'enabled', $control_rate = 0, $explore_rate = .2) {
    // The selection-mode argument can only be "random" or "adaptive", so coerce the
    // $decision_mode variable if it's somehow something else.
    if ($decision_style !== "adaptive") {
      $decision_style = "random";
    }
    $status = self::mapStatus($status);
    $path = "/agent-api/$machine_name";
    $agent = array(
      'name' => $label,
      'selection-mode' => $decision_style,
      'status' => $status,
      'control-rate' => $control_rate,
      'decision-stickiness' => 'session'
    );
    // Only include explore-rate for adaptive agents.
    if ($decision_style == 'adaptive') {
      $agent['explore-rate'] = $explore_rate;
    }
    $vars = array('agent' => $machine_name);
    $success_msg = 'The personalization {agent} was pushed to Acquia Lift';
    $fail_msg = 'The personalization {agent} could not be pushed to Acquia Lift';
    $this->makePutRequest($this->generateEndpoint($path), $this->getPutHeaders(), $agent, $success_msg, $fail_msg, $vars);
  }

  /**
   * Saves a decision point for an agent.
   *
   * @param $agent_name
   *   The name of the agent to save the point on.
   * @param $point
   *   The name of the decision point.
   */
  public function savePoint($agent_name, $point_name) {
    $path = "/agent-api/$agent_name/points/$point_name";
    $vars = array('agent' => $agent_name, 'decpoint' => $point_name);
    $success_msg = 'The point {decpoint} was pushed to the Acquia Lift personalization {agent}';
    $fail_msg = 'Could not save the point {decpoint} to the Acquia Lift personalization {agent}';
    $this->makePutRequest($this->generateEndpoint($path), $this->getPutHeaders(), NULL, $success_msg, $fail_msg, $vars);
  }

  /**
   * Saves a decision for an agent.
   *
   * @param $agent_name
   *   The name of the agent the decision belongs to.
   * @param $point
   *   The name of the decision point that the decision belongs to.
   * @param $decision_name
   *   The name of the decision to save.
   */
  public function saveDecision($agent_name, $point_name, $decision_name) {
    $path = "/agent-api/$agent_name/points/$point_name/decisions/$decision_name";
    $vars = array('agent' => $agent_name, 'decpoint' => $point_name, 'decname' => $decision_name);
    $success_msg = 'The decision {decname} for point {decpoint} was pushed to the Acquia Lift personalization {agent}';
    $fail_msg = 'Could not save decision {decname} for point {decpoint} to the Acquia Lift personalization {agent}';
    $this->makePutRequest($this->generateEndpoint($path), $this->getPutHeaders(), NULL, $success_msg, $fail_msg, $vars);
  }

  /**
   * Saves a choice for a decision.
   *
   * @param $agent_name
   *   The name of the agent the decision belongs to.
   * @param $point
   *   The name of the decision point containing the decision the choice
   *   belongs to.
   * @param $decision_name
   *   The name of the decision that the choice belongs to.
   * @param $choice
   *   The name of the choice to save.
   */
  public function saveChoice($agent_name, $point_name, $decision_name, $choice) {
    $path = "/agent-api/$agent_name/points/$point_name/decisions/$decision_name/choices/$choice";
    $vars = array('agent' => $agent_name, 'decpoint' => $point_name, 'choicename' => $decision_name . ': ' . $choice);
    $success_msg = 'The decision choice {choicename} for point {decpoint} was pushed to the Acquia Lift personalization {agent}';
    $fail_msg = 'Could not save decision choice {choicename} for point {decpoint} to the Acquia Lift personalization {agent}';
    $this->makePutRequest($this->generateEndpoint($path), $this->getPutHeaders(), NULL, $success_msg, $fail_msg, $vars);
  }

  /**
   * Resets the data for an agent.
   *
   * @param $agent_name
   */
  public function resetAgentData($agent_name) {
    $url = $this->generateEndpoint("/$agent_name/data");
    $response = $this->httpClient()->delete($url);
    $vars = array('agent' => $agent_name);
    $success_msg = 'The data for Acquia Lift personalization {agent} was reset';
    $fail_msg = 'Could not reset data for Acquia Lift personalization {agent}';
    if ($response->code == 200) {
      $this->logger()->log(PersonalizeLogLevel::INFO, $success_msg, $vars);
    }
    else {
      $this->handleBadResponse($response->code, $fail_msg, $vars);
    }
  }

  /**
   * Deletes an agent.
   *
   * @param $agent_name
   *   The name of the agent to delete.
   */
  public function deleteAgent($agent_name) {
    $path = "/agent-api/$agent_name";
    $vars = array('agent' => $agent_name);
    $success_msg = 'The Acquia Lift personalization {agent} was deleted';
    $fail_msg = 'Could not delete Acquia Lift personalization {agent}';
    $this->makeDeleteRequest($this->generateEndpoint($path), array(), $success_msg, $fail_msg, $vars);
  }

  /**
   * Saves a goal for an agent.
   *
   * @param $agent_name
   *   The agent the goal belongs to.
   * @param $goal_name
   *   The name of the goal.
   * @param array $data
   *   Array containing further information about the goal.
   */
  public function saveGoal($agent_name, $goal_name) {
    $path = "/agent-api/$agent_name/goals/$goal_name";
    $vars = array('agent' => $agent_name, 'goalname' => $goal_name);
    $success_msg = 'The goal {goalname} was pushed to the Acquia Lift personalization {agent}';
    $fail_msg = 'Could not save the goal {goalname} to the Acquia Lift personalization {agent}';
    $this->makePutRequest($this->generateEndpoint($path), $this->getPutHeaders(), NULL, $success_msg, $fail_msg, $vars);
  }

  /**
   * Deletes a goal from an agent.
   *
   * @param $agent_name
   *   The agent to delete the goal from.
   * @param $goal_name
   *   The name of the goal.
   */
  public function deleteGoal($agent_name, $goal_name) {
    $path = "/agent-api/$agent_name/goals/$goal_name";
    $vars = array('agent' => $agent_name, 'goalname' => $goal_name);
    $success_msg = 'The goal {goalname} was deleted from the Acquia Lift personalization {agent}';
    $fail_msg = 'Could not delete the goal {goalname} from the Acquia Lift personalization {agent}';
    $this->makeDeleteRequest($this->generateEndpoint($path), $this->getStandardHeaders(), $success_msg, $fail_msg, $vars);
  }

  /**
   * Retrieves the specified agent from Acquia Lift.
   *
   * @param $machine_name
   *   The machine name of the agent to retrieve.
   *
   * @return bool|array
   *   An array representing the agent or FALSE if none was found.
   */
  public function getAgent($machine_name) {
    $response = $this->makeGetRequest($this->generateEndpoint("/agent-api/$machine_name"), $this->getStandardHeaders(), 'Could not retrieve agent from Acquia Lift');
    return json_decode($response->data, TRUE);
  }

  /**
   * Gets a list of goals for the specified agent.
   *
   * @param $agent_name
   *   The name of the agent.
   * @return bool|mixed
   *   An array of goal names or FALSE if an error occurs.
   */
  public function getGoalsForAgent($agent_name) {
    $response = $this->makeGetRequest($this->generateEndpoint("/agent-api/$agent_name/goals"), $this->getStandardHeaders(), 'Could not retrieve goals from Acquia Lift');
    return json_decode($response->data, TRUE);
  }


  /**
   * Implements AcquiaLiftAPIInterface::getExistingAgentNames().
   */
  public function getExistingAgentNames() {
    $existing = $this->getAgents();
    return array_keys($existing);
  }

  /**
   * Retrieves a list of existing agents from Acquia Lift.
   *
   * @return array
   *   An associative array whose keys are agent names and values are objects
   *   representing agents.
   * @throws AcquiaLiftException
   */
  public function getAgents() {
    $response = $this->makeGetRequest($this->generateEndpoint("/list-agents"), $this->getStandardHeaders(), 'Error retrieving agent list from Acquia Lift');
    $response = json_decode($response->data, TRUE);
    if (!isset($response['data']['agents'])) {
      return array();
    }
    $existing_agents = array();
    foreach ($response['data']['agents'] as $agent) {
      $existing_agents[$agent['code']] = $agent;
    }
    return $existing_agents;
  }
  /**
   * Implements AcquiaLiftReportDataSourceInterface::getAgentStatusReport().
   */
  public function getAgentStatusReport($agent_names, $num_days = NULL, $as_json = FALSE) {
    $codes = implode(',', $agent_names);
    $days = (is_null($num_days) || !is_numeric($num_days)) ? '' : '&days=' . $num_days;
    $url = $this->generateEndpoint("/report/status?codes={$codes}{$days}");
    $response = $this->httpClient()->get($url, array('Accept' => 'application/json'));
    if ($response->code != 200) {
      $this->handleBadResponse($response->code, 'Problem retrieving status report.');
      return $as_json ? '' : array();
    }
    return $as_json ? $response->data : json_decode($response->data, TRUE);
  }

  /**
   * Implements AcquiaLiftReportDataSourceInterface::getConfidenceReport().
   */
  public function getConfidenceReport($agent_name, $date_start = NULL, $date_end = NULL, $point = NULL, $options = array(), $as_json = FALSE) {
    $default_options = array(
      'confidence-measure' => 0.95,
      'aggregated-over-dates' => true,
    );
    $options = array_merge($default_options, $options);
    $date_str = $this->getDateString($date_start, $date_end);
    $features = self::NO_FEATURES;
    // Provide legacy support for passing features.
    if (isset($options['features'])) {
      if ($options['features'] === 'all') {
        $features = '';
      }
      else {
        $features = is_array($options['features']) ? implode(',', $options['features']) : "(none)";
      }
      unset($options['features']);
    }

    $url = $this->generateEndpoint("/{$agent_name}/report/confidence{$date_str}?features=$features");
    foreach ($options as $param => $value) {
      $param_value = is_bool($value) ? var_export($value, TRUE) : (string)$value;
      $url .= "&$param=" . $param_value;
    }
    $headers = array('Accept' => 'application/json');
    if ($point !== NULL) {
      $headers['x-mpath-point'] = $point;
    }
    // Use a timeout of 8 seconds for retrieving the transform options.
    $response = $this->httpClient()->get($url, $headers);
    if ($response->code != 200) {
      $this->handleBadResponse($response->code, 'Problem retrieving confidence report.');
      return $as_json ? '' : array();
    }
    return $as_json ? $response->data : json_decode($response->data, TRUE);
  }

  /**
   * Implements AcquiaLiftReportDataSourceInterface::getRawLearningReport().
   */
  public function getRawLearningReport($agent_name, $date_start = NULL, $date_end = NULL, $point = NULL, $as_json = FALSE) {
    $date_str = $this->getDateString($date_start, $date_end);
    $url = $this->generateEndpoint("/{$agent_name}/report/learning{$date_str}");
    $headers = array('Accept' => 'application/json');
    if ($point !== NULL) {
      $headers['x-mpath-point'] = $point;
    }
    // Use a timeout of 8 seconds for retrieving the transform options.
    $response = $this->httpClient()->get($url, $headers);

    if ($response->code != 200) {
      $this->handleBadResponse($response->code, 'Problem retrieving learning report.');
      return $as_json ? '' : array();
    }
    return $as_json ? $response->data : json_decode($response->data, TRUE);
  }

  /**
   * Returns the number or runtime API calls that were made during the specified period.
   *
   * @param $date_start
   *   The start date in the format YYYY-MM-DD.
   * @param $date_end
   *   The end date in the format YYYY-MM-DD.
   * @return int
   */
  public function getAPICallsForPeriod($date_start, $date_end) {
    $date_str = $this->getDateString($date_start, $date_end);
    $url = $this->generateEndpoint("/-/report/system-usage{$date_str}");
    $headers = array('Accept' => 'application/json');
    $response = $this->httpClient()->get($url, $headers);
    if ($response->code != 200) {
      $this->handleBadResponse($response->code, 'Problem retrieving API call counts.');
      return array();
    }
    $result = json_decode($response->data, TRUE);
    if (!isset($result['data']) || !isset($result['data'][0]) || !isset($result['data'][0]['calls'])) {
      return array();
    }
    return $result['data'][0]['calls'];
  }

  /*
   * Converts an associative array of call counts to a count of the total number of
   * calls. Calls categorized as "other" (reporting, admin calls) are excluded by
   * default.
   */
  protected function convertCallCountsToTotalCount($counts, $exclude = array('other')) {
    $total_count = 0;
    foreach ($counts as $type => $count) {
      if (in_array($type, $exclude)) {
        continue;
      }
      $total_count += $count;
    }
    return $total_count;
  }

  /**
   * Returns the counts of API calls for the month prior to the date provided.
   *
   * @param $timestamp
   *   The timestamp representing the date from which to calculate the previous
   *   month's API calls.
   *
   * @return array
   *   An associative array with type of call as keys and counts for each type
   *   as values, e.g.
   *   array(
   *     'decisions' => 1000,
   *     'goals' => 100,
   *     'expires' => 2,
   *     'webactions' => 0,
   *     'other' => 10
   *   )
   */
  public function getCallsForPreviousMonth($timestamp) {
    $date = getdate($timestamp);
    $current_month = $date['mon'];
    $current_month_year = $last_month_year = $date['year'];

    if ($current_month == 1) {
      $last_month = 12;
      $last_month_year = $current_month_year - 1;
    }
    else {
      $last_month = $current_month - 1;
      if ($last_month < 10) {
        $last_month = '0' . $last_month;
      }
    }
    // Get a timestamp for the first of the month in question.
    $ts_last_month = strtotime("$last_month/01/$last_month_year");
    // Use this timestamp to get the number of days in that month.
    $num_days_last_month = date('t', $ts_last_month);
    $date_start = $last_month_year . '-' . $last_month . '-01';
    $date_end = $last_month_year . '-' . $last_month . '-' . $num_days_last_month;
    $calls_last_month = $this->getAPICallsForPeriod($date_start, $date_end);
    return $calls_last_month;
  }

  /**
   * Returns the total number of runtimeAPI calls for the month prior to the date
   * provided.
   *
   * @param $timestamp
   *   The timestamp representing the date from which to calculate the previous
   *   month's API calls.

   * @return int
   */
  public function getTotalRuntimeCallsForPreviousMonth($timestamp) {
    $calls_last_month = $this->getCallsForPreviousMonth($timestamp);
    return $this->convertCallCountsToTotalCount($calls_last_month);
  }

  /**
   * Returns counts of API calls from the 1st to the date provided.
   *
   * @param $timestamp
   *   The timestamp representing the date up to which to show API calls
   *   from the start of that month. For example, passing in a timestamp
   *   representing the date October 17th 2013 would return the number of
   *   API calls made from October 1st 2013 to October 17th 2013.
   *
   * @return array
   *   An associative array with type of call as keys and counts for each type
   *   as values, e.g.
   *   array(
   *     'decisions' => 1000,
   *     'goals' => 100,
   *     'expires' => 2,
   *     'webactions' => 0,
   *     'other' => 10
   *   )
   */
  public function getCallsForMonthToDate($timestamp) {
    $date_start = date('Y', $timestamp) . '-' . date('m', $timestamp) . '-01';
    $date_end = date('Y-m-d', $timestamp);
    $calls_this_month = $this->getAPICallsForPeriod($date_start, $date_end);
    return $calls_this_month;
  }

  /**
   * Returns the total number of runtimeAPI calls for the month prior to the date
   * provided.
   *
   * @param $timestamp
   *   The timestamp representing the date from which to calculate the previous
   *   month's API calls.

   * @return int
   */
  public function getTotalRuntimeCallsForMonthToDate($timestamp) {
    $calls_last_month = $this->getCallsForMonthToDate($timestamp);
    return $this->convertCallCountsToTotalCount($calls_last_month);
  }

  /**
   * Returns the timeframe portion of a report API url for the specified dates.
   *
   * @param $date_start
   *   The start date in the format YYYY-MM-DD or null to use today's date.
   * @param null $date_end
   *   The end date in the format YYYY-MM-DD or null for a single date.
   * @return string
   *   A string in the format /{start-date}/{end-date}
   */
  protected function getDateString($date_start, $date_end) {
    if ($date_start === NULL || !preg_match('/\d{4}\-\d{2}\-\d{2}/', $date_start)) {
      $date_start = date('Y-m-d');
    }
    $date_str = '/' . $date_start;
    if ($date_end !== NULL && preg_match('/\d{4}\-\d{2}\-\d{2}/', $date_end)) {
      $date_str .= '/' . $date_end;
    }
    return $date_str;
  }


  /**
   * Maps the passed in status code to one consumable by the Lift service.
   *
   * If the passed in status code is already a string status code that's
   * consumable by Lift it just returns that code. If it's a numeric code
   * defined in personalize module it maps it to the corresponding string
   * code. If it's neither, an exception is thrown.
   *
   * @param $status
   *   The status to map.
   * @return string status
   *   The status code consumable by Lift.
   * @throws AcquiaLiftException
   */
  public static function mapStatus($status) {
    $status_map = array(
      PERSONALIZE_STATUS_NOT_STARTED => self::PAUSED_STATUS,
      PERSONALIZE_STATUS_SCHEDULED => self::PAUSED_STATUS,
      PERSONALIZE_STATUS_PAUSED => self::PAUSED_STATUS,
      PERSONALIZE_STATUS_RUNNING => self::ENABLED_STATUS,
      PERSONALIZE_STATUS_COMPLETED => self::STOPPED_STATUS
    );
    // Maybe we already have a Lift status string.
    if (in_array($status, $status_map)) {
      return $status;
    }
    // Otherwise map it to a Lift status.
    if (!isset($status_map[$status])) {
      throw new AcquiaLiftException('Unknown agent status: ' . $status);
    }
    return $status_map[$status];
  }
  /**
   * Determines whether the passed in string is valid as an owner code.
   *
   * @param $str
   *   The string to check
   * @return bool
   *   Returns FALSE if the string contains any invalid characters, TRUE
   *   otherwise.
   */
  public static function codeIsValid($str) {
    return (bool) preg_match('/^' . self::VALID_NAME_MATCH_PATTERN . '+$/', $str);
  }

}
