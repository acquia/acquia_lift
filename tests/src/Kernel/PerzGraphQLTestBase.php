<?php

namespace Drupal\Tests\acquia_perz\Kernel;

use Drupal\graphql\Entity\Server;
use Drupal\Tests\node\Traits\ContentTypeCreationTrait;
use Drupal\Tests\node\Traits\NodeCreationTrait;
use Drupal\Tests\user\Traits\UserCreationTrait;
use Drupal\Tests\taxonomy\Traits\TaxonomyTestTrait;
use Drupal\Tests\acquia_perz\Traits\CustomBlockTestTrait;
use Drupal\KernelTests\KernelTestBase;
use Symfony\Component\HttpFoundation\Request;

/**
 * {@inheritdoc}
 */
abstract class PerzGraphQLTestBase extends KernelTestBase {

  use NodeCreationTrait {
    getNodeByTitle as drupalGetNodeByTitle;
    createNode as drupalCreateNode;
  }
  use UserCreationTrait {
    createUser as drupalCreateUser;
    createRole as drupalCreateRole;
    createAdminRole as drupalCreateAdminRole;
  }
  use ContentTypeCreationTrait {
    createContentType as drupalCreateContentType;
  }

  use TaxonomyTestTrait;

  use CustomBlockTestTrait;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'node',
    'datetime',
    'user',
    'system',
    'block',
    'block_content',
    'filter',
    'field',
    'text',
    'language',
    'taxonomy',
    'typed_data',
    'graphql',
    'image',
    'acquia_perz',
  ];

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();

    $this->installSchema('system', 'sequences');
    $this->installSchema('node', 'node_access');
    $this->installSchema('user', ['users_data']);
    $this->installConfig('filter');
    $this->installConfig('node');
    $this->installConfig('graphql');
    $this->installConfig('image');
    $this->installConfig('acquia_perz');
    $this->installEntitySchema('node');
    $this->installEntitySchema('user');
    $this->installEntitySchema('block_content');
    $this->installEntitySchema('taxonomy_term');
    $this->installEntitySchema('graphql_server');
  }

  /**
   * Get remote entities from current graphQL endpoint and compare results with
   * expect entities.
   *
   * @param integer $page_size
   * @param array $view_modes
   * @param array $expected_entities
   */
  public function checkDiscoveryEndpoint($page_size, $view_modes, $expected_entities) {
    if (!($this->graphqlServer instanceof Server)) {
      throw new \LogicException('GraphQL server is not set.');
    }
    $this->setUpPerzEntityTypes($view_modes);
    $this->setUpPerzPageSize($page_size);
    $actual_entities = $this->getRemoteEntitiesByTypes(
      array_keys($view_modes)
    );

    foreach ($actual_entities as $actual_entity_type_id => $actual_uuids) {
      $expected_uuids = $expected_entities[$actual_entity_type_id];
      $this->assertCount(count($actual_uuids), $expected_uuids);
      foreach ($expected_uuids as $expected_uuid) {
        $this->assertContains($expected_uuid, $actual_uuids);
      }
    }
  }

  /**
   * Get remote entities by entity types.
   *
   * @param array $entity_types
   * List of entity types that should be categorized with uuids.
   *
   * @return array
   */
  protected function getRemoteEntitiesByTypes($entity_types) {
    $result = [];
    $graphql_query_body = $this->graphQLQuery;
    $graphql_query_variables = [
      'page' => 0
    ];
    do {
      $response = $this->sendQuery(
        $graphql_query_body,
        $this->graphqlServer,
        $graphql_query_variables
      );
      $json_content = json_decode($response->getContent(), TRUE);
      $page_info = $json_content['data']['discover_entities']['page_info'];
      $items = $json_content['data']['discover_entities']['items'];
      foreach ($items as $item) {
        if (in_array($item['entity_type_id'], $entity_types)) {
          if (isset($result[$item['entity_type_id']])
            && !in_array($item['entity_uuid'], $result[$item['entity_type_id']])) {
            $result[$item['entity_type_id']][] = $item['entity_uuid'];
          }
          else {
            $result[$item['entity_type_id']] = [$item['entity_uuid']];
          }
        }
      }
    }
    while ($graphql_query_variables['page'] = $page_info['next_page']);
    return $result;
  }

  /**
   * Create and return graphql server entity.
   *
   * @param string $label
   * @param string $name
   * @param string $schema
   * @param string $endpoint
   *
   * @return \Drupal\Core\Entity\EntityBase|\Drupal\Core\Entity\EntityInterface|\Drupal\graphql\Entity\Server
   * @throws \Drupal\Core\Entity\EntityStorageException
   */
  protected function createGraphqlServer($label, $name, $schema, $endpoint) {
    $graphql_server = Server::create();
    $graphql_server->set('label', $label);
    $graphql_server->set('name', $name);
    $graphql_server->set('schema', $schema);
    $graphql_server->set('endpoint', $endpoint);
    $graphql_server->save();
    return $graphql_server;
  }

  /**
   * Setup Perz configuration config.
   *
   * @param array $view_modes
   * The array of entity_types > bundles > view_modes.
   */
  protected function setUpPerzEntityTypes($view_modes) {
    $entity_settings = $this->config('acquia_perz.entity_config');
    $entity_settings->set('view_modes', $view_modes);
    $entity_settings->save();
  }

  /**
   * Setup Perz page size config.
   *
   * @param integer $view_modes
   * The page max size.
   */
  protected function setUpPerzPageSize($page_size) {
    $cis_settings = $this->config('acquia_perz.settings');
    $cis_settings->set('cis.discovery_enpoint_page_size', $page_size);
    $cis_settings->save();
  }

  /**
   * Sends graphql query and returns response.
   *
   * @param string $query
   * The graphql query.
   * @param \Drupal\graphql\Entity\ServerInterface $server
   * The graphql server.
   * @param array $variables
   * The graphql variables.
   *
   * @return mixed
   * The graphql response.
   * @throws \Exception
   */
  protected function sendQuery($query, $server, array $variables = []) {
    if (!($server instanceof Server)) {
      throw new \LogicException('Invalid server.');
    }
    $endpoint = $server->get('endpoint');
    $host = \Drupal::request()->getSchemeAndHttpHost();
    $endpoint = $host . $endpoint;
    $request = Request::create($endpoint, 'GET', [
      'query' => $query,
      'variables' => $variables,
    ]);

    return $this->container->get('http_kernel')->handle($request);
  }

}
