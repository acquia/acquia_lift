<?php

abstract class AcquiaLiftAPIBase implements AcquiaLiftAPIInterface, PersonalizeLoggerAwareInterface {

  const NAME_MAX_LENGTH = 50;

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
   * Makes a GET request to an API endpoint.
   *
   * @param $path
   *   The endpoint path.
   * @param $headers
   *   Any headers that need to be added.
   * @param $fail_msg
   *   The error message to provide if the request fails.
   * @return array
   *   The decoded response as an array.
   * @throws \AcquiaLiftException
   */
  protected function makeGetRequest($url, $headers, $fail_msg) {
    $response = $this->httpClient()->get($url, $headers);
    if ($response->code != 200) {
      $this->handleBadResponse($response->code, $fail_msg);
      return array();
    }
    return $response;
  }

  /**
   * Makes a PUT request to an API endpoint.
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
   * @return array
   *   The decoded response as an array.
   * @throws \AcquiaLiftException
   */
  protected function makePutRequest($url, $headers, $body, $success_msg, $fail_msg, $vars) {
    $response = $this->httpClient()->put($url, $headers, $body);
    if ($response->code == 200) {
      $this->logger()->log(PersonalizeLogLevel::INFO, $success_msg, $vars);
    }
    else {
      $this->handleBadResponse($response->code, $fail_msg, $vars);
    }
    return $response;
  }

  /**
   * Makes a POST request to an API endpoint.
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
   * @return array
   *   The decoded response as an array.
   * @throws \AcquiaLiftException
   */
  protected function makePostRequest($url, $headers = array(), $body = NULL, $success_msg = '', $fail_msg = '', $vars = array()) {
    $response = $this->httpClient()->post($url, $headers, $body);
    if ($response->code == 200) {
      $this->logger()->log(PersonalizeLogLevel::INFO, $success_msg, $vars);
    }
    else {
      $this->handleBadResponse($response->code, $fail_msg, $vars);
    }
    return $response;
  }
  /**
   * Makes a DELETE request to an API endpoint.
   *
   * @param $path
   *   The endpoint path.
   * @param $headers
   *   Any headers that need to be added.
   * @param $success_msg
   *   The error message to provide if the request fails.
   * @param $fail_msg
   *   The error message to provide if the request fails.
   * @return array
   *   The decoded response as an array.
   * @throws \AcquiaLiftException
   */
  protected function makeDeleteRequest($url, $headers, $success_msg, $fail_msg, $vars) {
    $response = $this->httpClient()->delete($url, $headers);
    if ($response->code == 200) {
      $this->logger()->log(PersonalizeLogLevel::INFO, $success_msg, $vars);
    }
    else {
      // Delete requests should fail softly, i.e. just with logging, not throwing
      // an exception.
      $this->handleBadResponse($response->code, $fail_msg, $vars, TRUE, TRUE);
    }
    return $response;
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
    return $agent_name . $suffix;
  }
}
