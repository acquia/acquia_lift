<?php

namespace Drupal\Tests\acquia_perz\Kernel;

/**
 * Tests for discovery endpoint.
 *
 * @group node
 */
class DiscoveryEndpointTest extends PerzGraphQLTestBase {

  /**
   * {@inheritdoc}
   */
  protected $strictConfigSchema = FALSE;

  /**
   * {@inheritdoc}
   */
  protected $graphQLQuery = <<<GQL
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

  /**
   * {@inheritdoc}
   */
  protected $graphqlServer = NULL;

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();
    $this->graphqlServer = $this->createGraphqlServer(
      'Perz',
      'perz',
      'export_entities',
      '/perz3'
    );
    // Create current user and setup graphql permission.
    $this->setUpCurrentUser([], ["execute {$this->graphqlServer->id()} arbitrary graphql requests"]);
  }

  /**
   * Creates four nodes and ensures that they are loaded correctly.
   */
  public function testDiscoveryEndpoint() {
    // Create 2 nodes.
    $node1 = $this->drupalCreateNode(['type' => 'article', 'promote' => 1]);
    $node2 = $this->drupalCreateNode(['type' => 'article', 'promote' => 1]);

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

    $graphql_query_variables = [
      'page' => 0
    ];

    // 1. Use case when no entities are selected on perz form -
    // empty result with exception message should be returned.
    $this->setUpPerzEntityTypes([]);
    $result = $this->sendQuery(
      $this->graphQLQuery,
      $this->graphqlServer,
      $graphql_query_variables
    );
    $this->assertEquals(200, $result->getStatusCode());
    $json_content = json_decode($result->getContent(), TRUE);
    $this->assertNull($json_content['data']['discover_entities']);
    $this->assertEquals("No entity types are available for the export.",
      $json_content['extensions'][0]['message']);

    // 2. Use case when only nodes are selected on perz form. Expect 2 nodes
    // to be returned.
    $entity_types = [
      'node' => [
        'article' => [
          'default' => 1,
        ],
        'page' => [
          'default' => 1,
        ],
      ],
    ];
    $expected_entities = [
      'node' => [
        $node1->uuid(),
        $node2->uuid(),
      ],
    ];
    // Check for different page max sizes.
    $this->checkDiscoveryEndpoint(1, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(2, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(3, $entity_types, $expected_entities);

    // 3. Use case when only nodes and terms are selected on perz form -
    // expect 2 nodes and 4 terms to be returned. Also check that all
    // entities are unique.
    $entity_types = [
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
      ]
    ];
    $expected_entities = [
      'node' => [
        $node1->uuid(),
        $node2->uuid(),
      ],
      'taxonomy_term' => [
        $term1->uuid(),
        $term2->uuid(),
        $term3->uuid(),
        $term4->uuid(),
      ],
    ];
    // Check for different page max sizes.
    $this->checkDiscoveryEndpoint(1, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(2, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(3, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(4, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(5, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(6, $entity_types, $expected_entities);

    // 4. Add one more entity type - expect 9 entities.
    $entity_types = [
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
    ];
    $expected_entities = [
      'node' => [
        $node1->uuid(),
        $node2->uuid(),
      ],
      'taxonomy_term' => [
        $term1->uuid(),
        $term2->uuid(),
        $term3->uuid(),
        $term4->uuid(),
      ],
      'block_content' => [
        $custom_block1->uuid(),
        $custom_block2->uuid(),
        $custom_block3->uuid(),
      ]
    ];
    // Check for different page max sizes.
    $this->checkDiscoveryEndpoint(1, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(2, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(3, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(4, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(5, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(6, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(7, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(8, $entity_types, $expected_entities);
    $this->checkDiscoveryEndpoint(9, $entity_types, $expected_entities);
  }

}
