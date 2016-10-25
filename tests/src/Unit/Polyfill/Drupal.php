<?php

/**
 * This file polyfills all the functions that are defined in global namespace
 * in Drupal environment but not available in PHPUnit context.
 */

namespace {
  use Drupal\Tests\acquia_lift\Unit\Polyfill\Drupal\ImageStyleOptions;

  /**
   * Mock Drupal's t function.
   *
   * @param $string String to be translated.
   * @param array $args An array in the form ['from' => 'to', ...].
   * @return string
   */
  if (!function_exists('t')) {
    function t($string, array $args = []) {
      return strtr($string, $args);
    }
  }

  /**
   * Mock Drupal's image_style_options function.
   *
   * @param bool $include_empty
   * @return array
   */
  if (!function_exists('image_style_options')) {
    function image_style_options($include_empty = TRUE) {
      return ImageStyleOptions::$return;
    }
  }

  /**
   * Mock Drupal's file_create_url function.
   *
   * @param string $uri
   * @return string
   */
  if (!function_exists('file_create_url')) {
    function file_create_url($uri) {
      return 'file_create_url:' . $uri;
    }
  }
}

namespace Drupal\Tests\acquia_lift\Unit\Polyfill\Drupal {
  /**
   * Class ImageStyleOptions to manage image_style_options.
   */
  class ImageStyleOptions {
    /**
     * @var array image_style_options function's return value.
     */
    public static $return = [];
  }
}

namespace GuzzleHttp\Exception {
  class RequestException extends \Exception {};
}

namespace GuzzleHttp {
  use GuzzleHttp\Exception\RequestException;

  /**
   * Class Client.
   */
  class Client {
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
