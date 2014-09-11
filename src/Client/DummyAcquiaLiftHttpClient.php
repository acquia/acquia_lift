<?php

namespace Drupal\acquia_lift\Client;

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
  protected $requests_received;

  /**
   * Whether or not this http client should return 500 errors.
   *
   * @var bool
   */
  protected $broken;

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
    $response->code = $this->broken ? 500 : 200;
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
   */
  public function __construct($broken = FALSE, $data = array()) {
    $this->broken = $broken;
    $this->data = $data;
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
    $this->requests_received[] = array(
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
  public function getLoggedRequests() {
    return $this->requests_received;
  }

  public function removeLoggedRequests() {
    $this->requests_received = array();
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::get().
   */
  public function get($uri = null, $headers = null, array $options = array())
  {
    $this->logRequest('get', $uri, $headers, $options);

    $data = array('data' => array());
    if (strpos($uri, 'list-agents') !== FALSE) {
      $data['data']['agents'] = isset($this->data['agents']) ? $this->data['agents'] : array();
    }
    elseif (strpos($uri, 'transforms-options') !== FALSE) {
      $data['data']['options'] = isset($this->data['options']) ? $this->data['options'] : array();
    }
    elseif (strpos($uri, 'potential-targeting') !== FALSE) {
      $data['data']['potential'] = array(
        'features' => isset($this->data['features']) ? $this->data['features'] : array()
      );
    }
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
