<?php

namespace Drupal\Tests\acquia_perz\Kernel;

use Drupal\graphql\Entity\Server;
use Drupal\Tests\node\Traits\ContentTypeCreationTrait;
use Drupal\Tests\node\Traits\NodeCreationTrait;
use Drupal\Tests\user\Traits\UserCreationTrait;
use Drupal\Tests\taxonomy\Traits\TaxonomyTestTrait;
use Drupal\block_content\Entity\BlockContent;
use Drupal\block_content\Entity\BlockContentType;
use Drupal\node\Entity\Node;
use Drupal\KernelTests\KernelTestBase;
use Drupal\Tests\graphql\Traits\DataProducerExecutionTrait;
use Drupal\Tests\graphql\Traits\MockingTrait;
use Drupal\Tests\graphql\Traits\HttpRequestTrait;
use Drupal\Tests\graphql\Traits\QueryFileTrait;
use Drupal\Tests\graphql\Traits\QueryResultAssertionTrait;
use Drupal\Tests\graphql\Traits\SchemaPrinterTrait;
use Symfony\Component\HttpFoundation\Request;
use Drupal\user\Entity\Role;
use Drupal\user\RoleInterface;

/**
 * Tests the loading of multiple nodes.
 *
 * @group node
 */
class EntitiesTest extends KernelTestBase {

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
    'acquia_perz',
  ];

  protected $strictConfigSchema = FALSE;

  protected $blockType = NULL;

  protected $graphqlServer = NULL;

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();
    $this->installSchema('system', 'sequences');
    $this->installSchema('node', 'node_access');
    $this->installSchema('user', ['users_data']);
    $this->installEntitySchema('user');
    $this->installEntitySchema('node');
    $this->installEntitySchema('block_content');
    $this->installConfig('filter');
    $this->installConfig('node');
    $this->installConfig('graphql');
    $this->installConfig(['acquia_perz']);
    $this->installEntitySchema('node');
    $this->installEntitySchema('user');
    $this->installEntitySchema('taxonomy_term');
    $this->installEntitySchema('graphql_server');

    $this->graphqlServer = $this->createGraphqlServer();
  }

  /**
   * Creates four nodes and ensures that they are loaded correctly.
   */
  public function testNodeMultipleLoad() {
    // Create 2 nodes and 2 users.
    $node1 = $this->drupalCreateNode(['type' => 'article', 'promote' => 1]);
    $node2 = $this->drupalCreateNode(['type' => 'article', 'promote' => 1]);

    // Create current user and setup graphql permission.
    $this->setUpCurrentUser([], ["execute {$this->graphqlServer->id()} arbitrary graphql requests"]);

    // 4 terms.
    $vocabulary = $this->createVocabulary();
    $term1 = $this->createTerm($vocabulary);
    $term2 = $this->createTerm($vocabulary);
    $term3 = $this->createTerm($vocabulary);
    $term4 = $this->createTerm($vocabulary);

    // 3 custom blocks.
    $block_type = $this->createCustomBlockType();
    $custom_block1 = $this->createCustomBlock($block_type);
    $custom_block2 = $this->createCustomBlock($block_type);
    $custom_block3 = $this->createCustomBlock($block_type);
    /*[
      'node' => [
        'article' => [
          'default' => 1,
        ],
        'page' => [
          'default' => 1,
        ],
      ],
      'taxonomy_term' => [
        $vocabulary->id() => [
          'default' => 1,
        ]
      ],
      'block_content' => [
        $block_type->id() => [
          'default' => 1,
        ]
      ],
      'user' => [
        'user' => [
          'default' => 1,
        ]
      ],
    ]
    $this->setUpPerzPageSize(20);
    */


    $graphql_query_body = $this->getDiscoveryEndpointQuery();
    $graphql_query_variables = [
      'page' => 0
    ];
    $this->setUpPerzEntityTypes([]);
    $result = $this->sendQuery(
      $graphql_query_body,
      $this->graphqlServer,
      $graphql_query_variables
    );
    $this->assertEquals(200, $result->getStatusCode());
    $json_content = json_decode($result->getContent(), TRUE);
    $this->assertTrue(empty($json_content['data']['discover_entities']), 'Check that entities should not be there.');
    $arr = [];
    //print_r('<pre>'.print_r($json_content, TRUE).'</pre>');
    $page_info = $json_content['data']['discover_entities']['page_info'];
    foreach ($json_content['data']['discover_entities']['items'] as $json_item) {
      $entity_type_id = $json_item['entity_type_id'];
      $entity_uuid = $json_item['entity_uuid'];
      $entity = $this->container->get('entity.repository')->loadEntityByUuid($entity_type_id, $entity_uuid);
      $arr[] = [
        'entity_type_id' => $entity_type_id,
        'entity_uuid' => $entity_uuid,
        'entity_id' => $entity->id(),
        'label' => $entity->label(),
      ];
    }
    print_r('<pre>'.print_r($arr, TRUE).'</pre>');
    $this->assertEquals(200, $result->getStatusCode());

  }

  protected function createGraphqlServer() {
    $graphql_server = Server::create();
    $graphql_server->set('label', 'Perz');
    $graphql_server->set('name', 'perz');
    $graphql_server->set('schema', 'export_entities');
    $graphql_server->set('endpoint', '/perz3');
    $graphql_server->save();
    return $graphql_server;
  }

  protected function getDiscoveryEndpointQuery() {
    $graphql_query_body = <<<GQL
      query discoverEntities(\$page: Int! = 0) {
        discover_entities(page: \$page) {
          page_info {
            total_count
            current_page_count
            current_page
            next_page
            prev_page
          }
          items {
            entity_type_id
            entity_uuid
          }
        }
      }
    GQL;
    return $graphql_query_body;
  }

  protected function setUpPerzEntityTypes($view_modes) {
    $entity_settings = $this->config('acquia_perz.entity_config');
    $entity_settings->set('view_modes', $view_modes);
    $entity_settings->save();
  }
  protected function setUpPerzPageSize($page_size) {
    $cis_settings = $this->config('acquia_perz.settings');
    $cis_settings->set('cis.discovery_enpoint_page_size', $page_size);
    $cis_settings->save();
  }

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

  protected function createCustomBlockType() {
    $block_type = BlockContentType::create([
      'id' => 'test_block_type',
      'label' => 'Test block type',
      'description' => "Provides a test block type",
    ]);
    $block_type->save();
    return $block_type;
  }

  protected function createCustomBlock($block_type) {
    $custom_block = BlockContent::create([
      'type' => $block_type->id(),
      'info' => $this->randomMachineName(32),
      'body' => [
        'value' => $this->randomMachineName(64),
        'format' => 'full_html',
      ],
    ]);
    $custom_block->save();
    return $custom_block;
  }

}
