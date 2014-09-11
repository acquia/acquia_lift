<?php

/**
 * This is just an OOP wrapper around drupal_http_request.
 */
class AcquiaLiftDrupalHttpClient implements AcquiaLiftDrupalHttpClientInterface {

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
   * Implements AcquiaLiftDrupalHttpClientInterface::get().
   */
  public function get($uri = NULL, $headers = NULL, array $options = array()) {
    $headers = $headers ? $headers : array();
    $options = array('method' => 'GET', 'headers' => $headers) + $options;
    return drupal_http_request($uri, $options);
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::put().
   */
  public function put($uri = NULL, $headers = NULL, $body = NULL, array $options = array()) {
    $data = ($body === NULL ? NULL : $this->encodeBody($body));
    $headers = $headers ? $headers : array();
    $options = array('method' => 'PUT', 'data' => $data, 'headers' => $headers) + $options;
    return drupal_http_request($uri, $options);
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::post().
   */
  public function post($uri = NULL, $headers = NULL, $body = NULL, array $options = array()) {
    $data = $body ? $this->encodeBody($body) : NULL;
    $headers = $headers ? $headers : array();
    $options = array('method' => 'POST', 'data' => $data, 'headers' => $headers) + $options;
    return drupal_http_request($uri, $options);
  }

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::delete().
   */
  public function delete($uri = null, $headers = null, $body = null, array $options = array()) {
    $data = $body ? $this->encodeBody($body) : NULL;
    $headers = $headers ? $headers : array();
    $options = array('method' => 'DELETE', 'data' => $data, 'headers' => $headers) + $options;
    return drupal_http_request($uri, $options);
  }
}

