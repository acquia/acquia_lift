<?php


/**
 * A simple interface for http clients.
 *
 * The point of this is to make our AcquiaLiftAPI class unit testable. It
 * is modeled after Guzzle's ClientInterface. The AcquiaLiftAPI class relies
 * on an object implementing this interface to make calls to the Acquia Lift
 * service. For tests we can pass in a dummy http client so that no calls
 * are actually made.
 *
 * This also makes our code more maintainable between D7 and D8.
 *
 * @see tests/acquia_lift.test
 */
interface AcquiaLiftDrupalHttpClientInterface {

  /**
   * Create a GET request for the client
   *
   * @param string|array     $uri     Resource URI
   * @param array|Collection $headers HTTP headers
   * @param array            $options Options to apply to the request. For BC compatibility, you can also pass a
   *                                  string to tell Guzzle to download the body of the response to a particular
   *                                  location. Use the 'body' option instead for forward compatibility.
   */
  public function get($uri = null, $headers = null, array $options = array());

  /**
   * Create a PUT request for the client
   *
   * @param string|array                        $uri     Resource URI
   * @param array|Collection                    $headers HTTP headers
   * @param string|resource|EntityBodyInterface $body    Body to send in the request
   * @param array                               $options Options to apply to the request
   */
  public function put($uri = null, $headers = null, $body = null, array $options = array());

  /**
   * Create a POST request for the client
   *
   * @param string|array                                $uri      Resource URI
   * @param array|Collection                            $headers  HTTP headers
   * @param array|Collection|string|EntityBodyInterface $postBody POST body. Can be a string, EntityBody, or
   *                                                    associative array of POST fields to send in the body of the
   *                                                    request. Prefix a value in the array with the @ symbol to
   *                                                    reference a file.
   * @param array                                       $options Options to apply to the request
   */
  public function post($uri = null, $headers = null, $body = null, array $options = array());
  /**
   * Create a DELETE request for the client
   *
   * @param string|array                        $uri     Resource URI
   * @param array|Collection                    $headers HTTP headers
   * @param string|resource|EntityBodyInterface $body    Body to send in the request
   * @param array                               $options Options to apply to the request
   */
  public function delete($uri = null, $headers = null, $body = null, array $options = array());
}
