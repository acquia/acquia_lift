<?php

namespace Drupal\acquia_perz2\Plugin\GraphQL\DataProducer\Custom;

use Drupal\Core\Cache\RefinableCacheableDependencyInterface;
use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;
use Symfony\Component\HttpFoundation\RequestStack;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;

/**
 * @DataProducer(
 *   id = "return_self_parameter",
 *   name = @Translation("Return Self Parameter"),
 *   description = @Translation("Simple producer that returns taken parameter."),
 *   produces = @ContextDefinition("string",
 *     label = @Translation("Just returned parameter")
 *   ),
 *   consumes = {
 *     "value" = @ContextDefinition("string",
 *       label = @Translation("String")
 *     )
 *   }
 * )
 */
class ReturnSelfParameter extends DataProducerPluginBase {

  /**
   * @param string $string
   *
   * @return mixed
   */
  public function resolve($value) {
    return $value;
  }

}
