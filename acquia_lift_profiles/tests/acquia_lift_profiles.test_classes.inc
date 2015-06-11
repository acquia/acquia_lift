<?php

require_once(dirname(__FILE__) . '/../../tests/acquia_lift.test_classes.inc');

/**
 * Classes used for testing.
 */
class DummyALProfilesHttpClient extends DummyAcquiaLiftHttpClient {

  /**
   * Implements AcquiaLiftDrupalHttpClientInterface::get().
   */
  public function get($uri = null, $headers = null, array $options = array())
  {
    $this->logRequest('get', $uri, $headers, $options);
    $data = array();
    if (strpos($uri, 'segments') !== FALSE) {
      $data = isset($this->data['segments']) ? $this->data['segments'] : array();
    }
    elseif (strpos($uri, 'events') !== FALSE) {
      $data = isset($this->data['events']) ? $this->data['events'] : array();
    }
    return $this->generateDummyResponse($data);
  }
}
