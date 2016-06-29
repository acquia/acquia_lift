<?php

/**
 * @file
 * Contains \Drupal\Tests\acquia_lift\Service\Context\PageContextTest.
 */

namespace Drupal\Tests\acquia_lift\Service\Context;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Context\PageContext;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;

require_once(__DIR__ . '/../../Traits/SettingsDataTrait.php');

/**
 * PageContextTest Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Context\PageContext
 * @group acquia_lift
 */
class PageContextTest extends UnitTestCase {

  use SettingsDataTrait;

  /**
   * @var \Drupal\Core\Config\ConfigFactoryInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $configFactory;

  /**
   * @var \Drupal\Core\Config\ImmutableConfig|\PHPUnit_Framework_MockObject_MockObject
   */
  private $settings;

  /**
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $entityTypeManager;

  /**
   * Taxonomy term storage.
   *
   * @var \Drupal\taxonomy\TermStorageInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $taxonomyTermStorage;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->configFactory = $this->getMock('Drupal\Core\Config\ConfigFactoryInterface');
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\ImmutableConfig')
      ->disableOriginalConstructor()
      ->getMock();
    $this->entityTypeManager = $this->getMock('Drupal\Core\Entity\EntityTypeManagerInterface');
    $this->taxonomyTermStorage = $this->getMock('Drupal\taxonomy\TermStorageInterface');

    $this->configFactory->expects($this->once())
      ->method('get')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($this->getValidCredentialSettings());
    $this->settings->expects($this->at(1))
      ->method('get')
      ->with('field_mappings')
      ->willReturn($this->getValidFieldMappingsSettings());
    $this->settings->expects($this->at(2))
      ->method('get')
      ->with('thumbnail')
      ->willReturn($this->getValidThumbnailSettings());
    $this->entityTypeManager->expects($this->once())
      ->method('getStorage')
      ->with('taxonomy_term')
      ->willReturn($this->taxonomyTermStorage);
  }

  /**
   * Tests the getMetatags() method.
   *
   * @covers ::getMetatags
   */
  public function testGetMetatags() {
    $page_context = new PageContext($this->configFactory, $this->entityTypeManager);
    $metatags = $page_context->getMetatags();
    $expected_metatags = [
      'content_title' => 'Untitled',
      'content_type' => 'page',
      'page_type' => 'content page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '',
      'published_date' => '',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => '',
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ];

    $this->assertMetatagsRenderArray($expected_metatags, $metatags);
  }

  /**
   * testGetMetatagsWithSetByNode(), sub routine "set up thumbnail url".
   *
   * @param $node Node
   */
  private function testGetMetatagsWithSetByNodeSetUpThumbnailUrl($node) {
    $field_media = $this->getMockBuilder('Drupal\Core\Entity\ContentEntityInterface')
      ->disableOriginalConstructor()
      ->getMock();
    $field_image = $this->getMockBuilder('Drupal\Core\Entity\ContentEntityInterface')
      ->disableOriginalConstructor()
      ->getMock();
    $media_entity = $this->getMock('Drupal\Core\Entity\EntityInterface');
    $image_entity = $this->getMock('Drupal\file\FileInterface');

    $node->field_media = $field_media;
    $node->field_media->entity = $media_entity;
    $node->field_media->entity->field_image = $field_image;
    $node->field_media->entity->field_image->entity = $image_entity;

    $entity_manager = $this->getMock('Drupal\Core\Entity\EntityManagerInterface');
    $entity_storage = $this->getMock('Drupal\Core\Entity\EntityStorageInterface');
    $container = $this->getMock('Drupal\Core\DependencyInjection\Container');
    $image_style = $this->getMockBuilder('Drupal\image\Entity\ImageStyle')
      ->disableOriginalConstructor()
      ->getMock();

    \Drupal::setContainer($container);
    $container->expects($this->any())
      ->method('get')
      ->with('entity.manager')
      ->willReturn($entity_manager);
    $entity_manager->expects($this->once())
      ->method('getEntityTypeFromClass')
      ->with('Drupal\image\Entity\ImageStyle')
      ->willReturn($image_entity);
    $image_entity->expects($this->once())
      ->method('bundle')
      ->willReturn('file');
    $image_entity->expects($this->once())
      ->method('getFileUri')
      ->willReturn('a_file_uri');
    $entity_manager->expects($this->once())
      ->method('getStorage')
      ->with($image_entity)
      ->willReturn($entity_storage);
    $entity_storage->expects($this->once())
      ->method('load')
      ->with('medium')
      ->willReturn($image_style);
    $image_style->expects($this->once())
      ->method('buildUrl')
      ->with('a_file_uri')
      ->willReturn('a_style_decorated_file_uri');
  }

  /**
   * testGetMetatagsWithSetByNode(), sub routine "setup fields".
   */
  private function testGetMetatagsWithSetByNodeSetUpFields() {
    $tracked_content_term_1 = $this->getTerm('Tracked Content Term Name 1', 'tracked_content_vocabulary');
    $tracked_keyword_term_1 = $this->getTerm('Tracked Keyword Term Name 1', 'tracked_keyword_vocabulary');
    $tracked_keyword_term_2 = $this->getTerm('Tracked Keyword Term Name 2', 'tracked_keyword_vocabulary');
    $discarded_term_1 = $this->getTerm('Untracked Term Name', 'untracked_vocabulary_id');
    $terms = [
      90210 => [
        $tracked_content_term_1,
        $tracked_keyword_term_1,
        $tracked_keyword_term_2,
        $discarded_term_1,
      ],
    ];
    $this->taxonomyTermStorage->expects($this->once())
      ->method('getNodeTerms')
      ->with([90210])
      ->willReturn($terms);
  }

  /**
   * Tests the getMetatags() method, with setByNode().
   *
   * @covers ::getMetatags
   * @covers ::setByNode
   */
  public function testGetMetatagsWithSetByNode() {
    $node = $this->getNode();
    $this->testGetMetatagsWithSetByNodeSetUpThumbnailUrl($node);
    $this->testGetMetatagsWithSetByNodeSetUpFields();

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager);
    $page_context->setByNode($node);
    $metatags = $page_context->getMetatags();
    $expected_metatags = [
      'content_title' => 'My Title',
      'content_type' => 'article',
      'page_type' => 'node page',
      'content_section' => 'Tracked Content Term Name 1',
      'content_keywords' => 'Tracked Keyword Term Name 1,Tracked Keyword Term Name 2',
      'post_id' => 90210,
      'published_date' => 'a_published_time',
      'thumbnail_url' => 'file_create_url:a_style_decorated_file_uri',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => 'a_username',
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ];

    $this->assertMetatagsRenderArray($expected_metatags, $metatags);
  }

  /**
   * Tests the getMetatags() method, with setPageContextTitle().
   *
   * @covers ::getMetatags
   * @covers ::setPageContextTitle
   */
  public function testGetAllWithSetPageContextTitle() {
    $page_context = new PageContext($this->configFactory, $this->entityTypeManager);

    // Test set markup title.
    $title = [
      '#markup' => '<div><a>My Page Title 1</a></div>',
      '#allowed_tags' => ['a'],
    ];
    $page_context->setPageContextTitle($title);
    $metatags = $page_context->getMetatags();
    $expected_metatags = [
      'content_title' => '<a>My Page Title 1</a>',
      'content_type' => 'page',
      'page_type' => 'content page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '',
      'published_date' => '',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => '',
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ];
    $this->assertMetatagsRenderArray($expected_metatags, $metatags);

    // Test set string title.
    $title = 'My Page Title 2';
    $page_context->setPageContextTitle($title);
    $all_page_context = $page_context->getMetatags();
    $expected_page_context = [
      'content_title' => 'My Page Title 2',
      'content_type' => 'page',
      'page_type' => 'content page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '',
      'published_date' => '',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => '',
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ];
    $this->assertMetatagsRenderArray($expected_page_context, $all_page_context);

    // Test set NULL title.
    $page_context->setPageContextTitle(NULL);
    $all_page_context = $page_context->getMetatags();
    $expected_page_context = [
      'content_title' => '',
      'content_type' => 'page',
      'page_type' => 'content page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '',
      'published_date' => '',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => '',
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ];
    $this->assertMetatagsRenderArray($expected_page_context, $all_page_context);
  }

  /**
   * Get Term.
   *
   * @param string $name
   * @param string $vocabulary_id
   *
   * @return Drupal\taxonomy\TermInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private function getTerm($name = 'Term Name', $vocabulary_id = 'untracked_vocabulary_id') {
    $term = $this->getMock('Drupal\taxonomy\TermInterface');
    $term->expects($this->once())
      ->method('getVocabularyId')
      ->willReturn($vocabulary_id);
    $term->expects($this->once())
      ->method('getName')
      ->willReturn($name);
    return $term;
  }

  /**
   * Get Node.
   *
   * @param integer $id
   *
   * @return Drupal\node\NodeInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private function getNode($id = 90210) {
    $user = $this->getUser();
    $field_country = $this->getMock('Drupal\Core\Field\BaseFieldDefinition');
    $field_tags = $this->getMock('Drupal\Core\Field\BaseFieldDefinition');
    $node = $this->getMock('Drupal\node\NodeInterface');
    $field_country_handler_settings = [
      'target_bundles' => [
        'tracked_content_vocabulary',
      ]
    ];
    $field_tags_handler_settings = [
      'target_bundles' => [
        'tracked_keyword_vocabulary',
      ]
    ];

    $node->expects($this->exactly(2))
      ->method('getType')
      ->willReturn('article');
    $node->expects($this->once())
      ->method('getTitle')
      ->willReturn('My Title');
    $node->expects($this->once())
      ->method('getCreatedTime')
      ->willReturn('a_published_time');
    $node->expects($this->any())
      ->method('id')
      ->willReturn($id);
    $node->expects($this->once())
      ->method('getOwner')
      ->willReturn($user);
    $field_country->expects($this->once())
      ->method('getSetting')
      ->with('handler_settings')
      ->willReturn($field_country_handler_settings);
    $field_tags->expects($this->once())
      ->method('getSetting')
      ->with('handler_settings')
      ->willReturn($field_tags_handler_settings);

    $node->field_country = $field_country;
    $node->field_tags = $field_tags;

    return $node;
  }

  /**
   * Get User.
   *
   * @param string $username
   *
   * @return Drupal\user\UserInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private function getUser($username = 'a_username') {
    $user = $this->getMock('Drupal\user\UserInterface');
    $user->expects($this->once())
      ->method('getUsername')
      ->willReturn($username);
    return $user;
  }

  /**
   * Asserts metatags render array.
   *
   * @param $expected_metatags
   * @param $metatags
   */
  private function assertMetatagsRenderArray($expected_metatags, $metatags) {
    $populated_metatags = [];

    foreach ($metatags as $metatag) {
      $renderArray = $metatag[0];
      $name = $metatag[1];

      $this->assertEquals('html_tag', $renderArray['#type']);
      $this->assertEquals('meta', $renderArray['#tag']);
      $this->assertEquals('acquia_lift:' . $name, $renderArray['#attributes']['itemprop']);

      $populated_metatags[$name] = $renderArray['#attributes']['content'];
    }

    $this->assertEquals($expected_metatags, $populated_metatags);
  }
}
