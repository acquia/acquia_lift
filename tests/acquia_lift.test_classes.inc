<?php
/**
 * @file
 * Provides test classes for Acquia Lift
 */

/**
 * Classes used for testing.
 */
class DummyAcquiaLiftHttpClient implements AcquiaLiftDrupalHttpClientInterface {

  /**
   * Stores all requests that have been received.
   *
   * @var array
   *   An array of requests.
   */
  protected static $requests_received;

  /**
   * Whether or not this http client should return 500 errors.
   *
   * @var bool
   */
  protected $broken;

    /**
     * The type of breakage to simulate, i.e. client or server side.
     *
     * @var string
     */
  protected $breakageType;

  /**
   * An array of data simulating resources than can be returned.
   *
   * @var array
   */
  protected $data = array();

  /**
   * Generates a dummy response based on the passed in data.
   *
   * @param array $data
   *   An array of data for the response.
   * @return stdClass
   *   An object representing a response from the server.
   */
  protected function generateDummyResponse($data) {
    $response = new stdClass();
    $response->code = 200;
    $response->status_message = 'OK';
    if ($this->broken) {
      if ($this->breakageType == 'client') {
        $response->code = 400;
        $response->status_message = 'Bad request';
      }
      else {
        $response->code = 500;
        $response->status_message = 'Internal Server Error';
      }
    }
    $response->data = drupal_json_encode($data);
    return $response;
  }

  /**
   * Constructor
   *
   * @param bool $broken
   *   Whether or not this http client should just get 500 errors.
   * @param array $data
   *   An array of dummy data that can be returned in responses.
   * @param bool $simulate_client_side_breakage
   *   Do simulate client side breakage.
   */
  public function __construct($broken = FALSE, $data = array(), $simulate_client_side_breakage = FALSE) {
    $this->broken = $broken;
    $this->breakageType = $simulate_client_side_breakage ? 'client' : 'server';
    $this->data += $data;
  }

  /**
   * Logs the request internally.
   *
   * @param $type
   *   The type of request, e.g. 'get'
   * @param $uri
   *   The uri of the request.
   * @param $headers
   *   The array of headers.
   * @param $options
   *   An array of options
   * @param null $body
   *   (optional) The body of the request.
   */
  protected function logRequest($type, $uri, $headers, $options, $body = NULL) {
    self::$requests_received[] = array(
      'type' => $type,
      'uri' => $uri,
      'headers' => $headers,
      'options' => $options,
      'body' => $body
    );
  }

  /**
   * Returns all requests that have been made to this client.
   *
   * @return array
   *   An array of requests
   */
  public static function getLoggedRequests() {
    return self::$requests_received;
  }

  public static function clearLoggedRequests() {
    self::$requests_received = array();
  }

  /**
   * Returns the expected data array for a given uri.
   *
   * @param string $uri
   *   An absolute url for an API endpoint, e.g. http://example.com/owner-code/list-agents
   * @return array
   *   An array of data to be returned in the response.
   */
  protected function getDataForURI($uri, $headers = array()) {
    $parsed = parse_url($uri);
    $path_parts = explode('/', $parsed['path']);
    // The first element of the $path_parts array will be an empty string.
    $path_parts = array_slice($path_parts, 1);

    switch($path_parts[0]) {
      case 'campaigns':
        if (isset($this->data['campaigns'])) {
          return $this->data['campaigns'];
        }
        if (isset($this->data['error'])) {
          return $this->data['error'];
        }
        return array();
      case 'report':
        $query_params = explode('&', $parsed['query']);
        $first_param = explode('=', reset($query_params));
        if ($first_param[0] !== 'campaign_id') {
          return array();
        }
        $agent_name = $first_param[1];
        return isset($this->data['reports'][$agent_name]) ? $this->data['reports'][$agent_name] : array();
      case 'ping':
        return array('message' => 'pong!');
        break;
    }
    return array();
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::get().
   */
  public function get($uri = null, $headers = null, array $options = array())
  {
    $this->logRequest('get', $uri, $headers, $options);
    $data = $this->getDataForURI($uri, $headers);
    return $this->generateDummyResponse($data);
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::put().
   */
  public function put($uri = null, $headers = null, $body = null, array $options = array())
  {
    $this->logRequest('put', $uri, $headers, $options, $body);
    return $this->generateDummyResponse(array('status' => 'ok'));
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::post().
   */
  public function post($uri = null, $headers = null, $body = null, array $options = array())
  {
    $this->logRequest('post', $uri, $headers, $options, $body);
    return $this->generateDummyResponse(array('status' => 'ok'));
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::delete().
   */
  public function delete($uri = null, $headers = null, $body = null, array $options = array())
  {
    $this->logRequest('delete', $uri, $headers, $options, $body);
    return $this->generateDummyResponse(array('status' => 'ok'));
  }

}

class AcquiaLiftTestLogger implements PersonalizeLoggerInterface {
  protected static $logs = array();
  protected $output = FALSE;

  public function __construct($output = FALSE) {
    $this->output = $output;
  }

  public function log($level, $message, array $context = array())
  {
    foreach ($context as $key => $value) {
      if (strpos($message, '{' . $key . '}') !== FALSE) {
        $message = str_replace('{' . $key . '}', $value, $message);
      }
    }
    if ($this->output && function_exists('drupal_set_message')) {
      drupal_set_message($message, $level);
    }
    self::$logs[] = array(
      'level' => $level,
      'message' => $message,
    );
  }

  public function clearLogs() {
    self::$logs = array();
  }

  public function getLogs() {
    return self::$logs;
  }
}

/**
 * Class AcquiaLiftTestReports
 *
 * Helper class providing static methods that return basic reports.
 */
class AcquiaLiftTestReports {

  public static function getBasicReport() {
    return array(
      'results' => array(
        array (
          'decision_id' => 'option-a',
          'results' => array(
            array(
              'timestamp' => '2015-07-07T00:00:00Z',
              'total_play' => 250,
              'total_goals' => 25,
              'total_goals_value' => 25,
              'total_plays_explore' => 100,
              'total_goals_explore' => 12,
              'total_goals_value_explore' => 12,
            ),
            array(
              'timestamp' => '2015-07-08T00:00:00Z',
              'total_play' => 240,
              'total_goals' => 26,
              'total_goals_value' => 26,
              'total_plays_explore' => 101,
              'total_goals_explore' => 13,
              'total_goals_value_explore' => 13,
            ),
          ),
        ),
        array(
          'decision_id' => 'option-b',
          'results' => array(
            array(
              'timestamp' => '2015-07-07T00:00:00Z',
              'total_play' => 265,
              'total_goals' => 27,
              'total_goals_value' => 27,
              'total_plays_explore' => 99,
              'total_goals_explore' => 11,
              'total_goals_value_explore' => 11,
            ),
            array(
              'timestamp' => '2015-07-08T00:00:00Z',
              'total_play' => 245,
              'total_goals' => 22,
              'total_goals_value' => 22,
              'total_plays_explore' => 100,
              'total_goals_explore' => 10,
              'total_goals_value_explore' => 10,
            ),
          ),
        ),
      ),
      'total_plays' => 1000,
      'total_goals' => 100,
    );
  }
}
