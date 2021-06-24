<?php

namespace Drupal\acquia_perz2\Plugin\GraphQL\DataProducer\Site;

use Drupal\Core\Cache\RefinableCacheableDependencyInterface;
use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;
use Symfony\Component\HttpFoundation\RequestStack;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;

/**
 * @DataProducer(
 *   id = "base_url",
 *   name = @Translation("Base URL"),
 *   description = @Translation("Base URL of the site."),
 *   produces = @ContextDefinition("string",
 *     label = @Translation("Base url")
 *   )
 * )
 */
class BaseUrl extends DataProducerPluginBase implements ContainerFactoryPluginInterface {

  /**
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $requestStack;

  /**
   * {@inheritdoc}
   *
   * @codeCoverageIgnore
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $container->get('request_stack')
    );
  }

  /**
   * Articles constructor.
   *
   * @param array $configuration
   *   The plugin configuration.
   * @param string $pluginId
   *   The plugin id.
   * @param mixed $pluginDefinition
   *   The plugin definition.
   * @param \Symfony\Component\HttpFoundation\RequestStack $request_stack
   *
   * @codeCoverageIgnore
   */
  public function __construct(
    array $configuration,
    $pluginId,
    $pluginDefinition,
    RequestStack $request_stack
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition);
    $this->requestStack = $request_stack;
  }

  /**
   * @param \Drupal\Core\Cache\RefinableCacheableDependencyInterface $metadata
   *
   * @return string
   *  The site's absolute base url.
   */
  public function resolve(RefinableCacheableDependencyInterface $metadata) {
    return $this->requestStack->getCurrentRequest()->getSchemeAndHttpHost() . '/';
  }

}
