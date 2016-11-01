<?php

/**
 * This file polyfills all the functions that are defined in global namespace
 * in Drupal environment but not available in PHPUnit context.
 */

namespace Drupal\Tests\acquia_lift\Unit\Polyfill\Drupal {
  /**
   * Class PolyfillController to manage polyfilled global functions.
   */
  class PolyfillController {
    /**
     * @var array manages functions' return values.
     */
    public static $return = [];
  }

}

namespace {
  use Drupal\Tests\acquia_lift\Unit\Polyfill\Drupal\PolyfillController;

  if (!function_exists('acquia_polyfill_controller_set_return')) {
    /**
     * Set polyfill return value.
     *
     * @param string $function_name
     *   The name of the function that is being polyfilled.
     * @param mixed
     *   Return value.
     */
    function acquia_polyfill_controller_set_return($function_name, $return_value) {
      PolyfillController::$return[$function_name] = $return_value;
    }
  }

  if (!function_exists('t')) {
    /**
     * Mock Drupal's t function.
     *
     * @param string $string
     *   String to be translated.
     * @param array $args
     *   An array in the form ['from' => 'to', ...].
     *
     * @return string
     *   Return value.
     */
    function t($string, array $args = []) {
      return strtr($string, $args);
    }
  }

  if (!function_exists('image_style_options')) {
    /**
     * Mock Drupal's image_style_options function.
     *
     * @param bool $include_empty
     *   Include empty if TRUE, don't include empty otherwise.
     *
     * @return array
     *   Return value.
     */
    function image_style_options($include_empty = TRUE) {
      return PolyfillController::$return['image_style_options'];
    }
  }

  if (!function_exists('file_create_url')) {
    /**
     * Mock Drupal's file_create_url function.
     *
     * @param string $uri
     *   URI.
     *
     * @return string
     *   Return value.
     */
    function file_create_url($uri) {
      return 'file_create_url:' . $uri;
    }
  }
}

namespace GuzzleHttp\Exception {
  class RequestException extends \Exception {}
}

namespace GuzzleHttp {
  use GuzzleHttp\Exception\RequestException;

  interface ClientInterface {
    const VERSION = '6.2.1';
  }

  /**
   * Class Client.
   */
  class Client implements ClientInterface {

    /**
     * @var array Data.
     */
    private $data;

    /**
     * Constructs an Client object.
     *
     * @param array
     *   Data.
     */
    public function __construct($data) {
      $this->data = $data;
    }

    /**
     * GET an end point.
     *
     * @param string $path
     *   Path to the end point.
     * @param array $config
     *   Config of the GET call.
     * @return Response
     *   Response.
     */
    public function get($path, $config) {
      if (empty($path)) {
        throw new RequestException();
      }

      return new Response(serialize($this->data) . ' ' . $path . ' ' . serialize($config));
    }
  }

  /**
   * Class Response.
   */
  class Response {
    /**
     * @var string Reason phrase.
     */
    private $reasonPhrase;

    /**
     * Constructs an Response object.
     *
     * @param string
     *   Reason phrase.
     */
    public function __construct($reasonPhrase) {
      $this->reasonPhrase = $reasonPhrase;
    }

    /**
     * Get status code.
     *
     * @return string
     *   Status code.
     */
    public function getStatusCode() {
      return 123;
    }

    /**
     * Get reason phrase.
     *
     * @return string
     *   Reason phrase.
     */
    public function getReasonPhrase() {
      return $this->reasonPhrase;
    }
  }
}
