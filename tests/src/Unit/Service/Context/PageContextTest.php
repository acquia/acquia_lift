<?php

namespace Drupal\Tests\acquia_lift\Unit\Service\Context;

use Drupal\acquia_lift\Service\Context\PageContext;
use Drupal\Tests\acquia_lift\Traits\SettingsDataTrait;
use Drupal\Tests\UnitTestCase;

require_once(__DIR__ . '/../../../Traits/SettingsDataTrait.php');
require_once(__DIR__ . '/../../Polyfill/Drupal.php');

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
   * Request stack.
   *
   * @var \Symfony\Component\HttpFoundation\RequestStack|\PHPUnit_Framework_MockObject_MockObject
   */
  private $requestStack;

  /**
   * Request.
   *
   * @var \Symfony\Component\HttpFoundation\Request|\PHPUnit_Framework_MockObject_MockObject
   */
  private $request;

  /**
   * Request's parameter bag.
   *
   * @var \Symfony\Component\HttpFoundation\ParameterBag|\PHPUnit_Framework_MockObject_MockObject
   */
  private $requestParameterBag;

  /**
   * Route match.
   *
   * @var \Drupal\Core\Routing\RouteMatchInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $routeMatch;

  /**
   * Route.
   *
   * @var \Symfony\Component\Routing\Route|\PHPUnit_Framework_MockObject_MockObject
   */
  private $route;

  /**
   * Title resolver.
   *
   * @var \Drupal\Core\Controller\TitleResolverInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $titleResolver;

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
    $this->requestStack = $this->getMock('Symfony\Component\HttpFoundation\RequestStack');
    $this->request = $this->getMock('Symfony\Component\HttpFoundation\Request');
    $this->requestParameterBag = $this->getMock('Symfony\Component\HttpFoundation\ParameterBag');
    $this->routeMatch = $this->getMock('Drupal\Core\Routing\RouteMatchInterface');
    $this->route = $this->getMockBuilder('Symfony\Component\Routing\Route')
      ->disableOriginalConstructor()
      ->getMock();
    $this->titleResolver = $this->getMock('Drupal\Core\Controller\TitleResolverInterface');

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
    $this->requestStack->expects($this->once())
      ->method('getCurrentRequest')
      ->willReturn($this->request);
    $this->request->attributes = $this->requestParameterBag;
    $this->routeMatch->expects($this->once())
      ->method('getRouteObject')
      ->willReturn($this->route);
  }

  /**
   * Tests the populateHtmlHead() method, credential configuration.
   *
   * @covers ::populateHtmlHead
   */
  public function testPopulateHtmlHeadCredentialConfiguration() {
    $this->requestParameterBag->expects($this->once())
      ->method('has')
      ->with('node')
      ->willReturn(FALSE);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver);
    $head = ['old_head'];
    $page_context->populateHtmlHead($head);

    $expected_head = $this->toRenderArray([
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
      'account_id' => 'account_id_1',
      'site_id' => 'site_id_1',
      'contentOrigin' => 'content_origin_1',
      'liftAssetsURL' => 'assets_url_1',
      'liftDecisionAPIURL' => 'https://example.com',
      'authEndpoint' => 'https://example.com',
    ], 'assets_url_1');

    $this->assertEquals($expected_head, $head);
  }

  /**
   * Tests the populateHtmlHead() method, with a Node.
   *
   * @covers ::populateHtmlHead
   */
  public function testPopulateHtmlHeadWithNode() {
    $this->requestParameterBag->expects($this->once())
      ->method('has')
      ->with('node')
      ->willReturn(TRUE);
    $this->requestParameterBag->expects($this->once())
      ->method('get')
      ->with('node')
      ->willReturn($this->getNode());

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver);
    $head = ['old_head'];
    $page_context->populateHtmlHead($head);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title',
      'content_type' => 'article',
      'page_type' => 'node page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '90210',
      'published_date' => 'a_published_time',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => 'a_username',
      'account_id' => 'account_id_1',
      'site_id' => 'site_id_1',
      'contentOrigin' => 'content_origin_1',
      'liftAssetsURL' => 'assets_url_1',
      'liftDecisionAPIURL' => 'https://example.com',
      'authEndpoint' => 'https://example.com',
    ], 'assets_url_1');

    $this->assertEquals($expected_head, $head);
  }

  /**
   * Tests the populateHtmlHead() method, with A Node and simple title.
   *
   * @covers ::populateHtmlHead
   */
  public function testPopulateHtmlHeadWithNodeAndSimpleTitle() {
    $this->requestParameterBag->expects($this->once())
      ->method('has')
      ->with('node')
      ->willReturn(TRUE);
    $this->requestParameterBag->expects($this->once())
      ->method('get')
      ->with('node')
      ->willReturn($this->getNode());
    $this->titleResolver->expects($this->once())
      ->method('getTitle')
      ->with($this->request, $this->route)
      ->willReturn('My Title from Title Resolver');

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver);
    $head = ['old_head'];
    $page_context->populateHtmlHead($head);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title from Title Resolver',
      'content_type' => 'article',
      'page_type' => 'node page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '90210',
      'published_date' => 'a_published_time',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => 'a_username',
      'account_id' => 'account_id_1',
      'site_id' => 'site_id_1',
      'contentOrigin' => 'content_origin_1',
      'liftAssetsURL' => 'assets_url_1',
      'liftDecisionAPIURL' => 'https://example.com',
      'authEndpoint' => 'https://example.com',
    ], 'assets_url_1');

    $this->assertEquals($expected_head, $head);
  }

  /**
   * Tests the populateHtmlHead() method, with A Node and array title.
   *
   * @covers ::populateHtmlHead
   */
  public function testPopulateHtmlHeadWithNodeAndArrayTitle() {
    $this->requestParameterBag->expects($this->once())
      ->method('has')
      ->with('node')
      ->willReturn(TRUE);
    $this->requestParameterBag->expects($this->once())
      ->method('get')
      ->with('node')
      ->willReturn($this->getNode());
    $this->titleResolver->expects($this->once())
      ->method('getTitle')
      ->with($this->request, $this->route)
      ->willReturn([
        '#markup' => 'My Title from Title Resolver <a><a/><script></script><br />',
        '#allowed_tags' => ['br'],
      ]);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver);
    $head = ['old_head'];
    $page_context->populateHtmlHead($head);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title from Title Resolver <br />',
      'content_type' => 'article',
      'page_type' => 'node page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '90210',
      'published_date' => 'a_published_time',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => 'a_username',
      'account_id' => 'account_id_1',
      'site_id' => 'site_id_1',
      'contentOrigin' => 'content_origin_1',
      'liftAssetsURL' => 'assets_url_1',
      'liftDecisionAPIURL' => 'https://example.com',
      'authEndpoint' => 'https://example.com',
    ], 'assets_url_1');

    $this->assertEquals($expected_head, $head);
  }

  /**
   * Tests the populateHtmlHead() method, with a Node and a thumbnail URL.
   *
   * @covers ::populateHtmlHead
   */
  public function testPopulateHtmlHeadWithNodeAndThumbnailUrl() {
    $node = $this->getNode();

    $this->requestParameterBag->expects($this->once())
      ->method('has')
      ->with('node')
      ->willReturn(TRUE);

    $this->requestParameterBag->expects($this->once())
      ->method('get')
      ->with('node')
      ->willReturn($node);

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
    $entity_manager->expects($this->once())
      ->method('getStorage')
      ->with($image_entity)
      ->willReturn($entity_storage);
    $entity_storage->expects($this->once())
      ->method('load')
      ->with('medium')
      ->willReturn($image_style);

    $image_entity->expects($this->once())
      ->method('bundle')
      ->willReturn('file');

    $image_entity->expects($this->once())
      ->method('getFileUri')
      ->willReturn('a_file_uri');

    $image_style->expects($this->once())
      ->method('buildUrl')
      ->with('a_file_uri')
      ->willReturn('a_style_decorated_file_uri');

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver);
    $head = ['old_head'];
    $page_context->populateHtmlHead($head);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title',
      'content_type' => 'article',
      'page_type' => 'node page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '90210',
      'published_date' => 'a_published_time',
      // This is broken. Help!
      'thumbnail_url' => 'file_create_url:a_style_decorated_file_uri',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => 'a_username',
      'account_id' => 'account_id_1',
      'site_id' => 'site_id_1',
      'contentOrigin' => 'content_origin_1',
      'liftAssetsURL' => 'assets_url_1',
      'liftDecisionAPIURL' => 'https://example.com',
      'authEndpoint' => 'https://example.com',
    ], 'assets_url_1');

    $this->assertEquals($expected_head, $head);
  }

  /**
   * Tests the populateHtmlHead() method, with a Node and fields.
   *
   * @covers ::populateHtmlHead
   */
  public function testPopulateHtmlHeadWithNodeAndFields() {
    $this->requestParameterBag->expects($this->once())
      ->method('has')
      ->with('node')
      ->willReturn(TRUE);
    $this->requestParameterBag->expects($this->once())
      ->method('get')
      ->with('node')
      ->willReturn($this->getNode());
    $this->populateHtmlHeadWithNodeAndFieldsSetUpFields();

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver);
    $head = ['old_head'];
    $page_context->populateHtmlHead($head);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title',
      'content_type' => 'article',
      'page_type' => 'node page',
      'content_section' => 'Tracked Content Term Name 1',
      'content_keywords' => 'Tracked Keyword Term Name 1,Tracked Keyword Term Name 2',
      'post_id' => '90210',
      'published_date' => 'a_published_time',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'author' => 'a_username',
      'account_id' => 'account_id_1',
      'site_id' => 'site_id_1',
      'contentOrigin' => 'content_origin_1',
      'liftAssetsURL' => 'assets_url_1',
      'liftDecisionAPIURL' => 'https://example.com',
      'authEndpoint' => 'https://example.com',
    ], 'assets_url_1');

    $this->assertEquals($expected_head, $head);
  }

  /**
   * Get Term.
   *
   * @param string $name
   * @param string $vocabulary_id
   *
   * @return \Drupal\taxonomy\TermInterface|\PHPUnit_Framework_MockObject_MockObject
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
   * @return \Drupal\node\NodeInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private function getNode($id = 90210) {
    $user = $this->getUser();
    $field_country = $this->getMock('Drupal\Core\Field\BaseFieldDefinition');
    $field_tags = $this->getMock('Drupal\Core\Field\BaseFieldDefinition');
    $node = $this->getMock('Drupal\node\NodeInterface');
    $field_country_handler_settings = [
      'target_bundles' => [
        'tracked_content_vocabulary',
      ],
    ];
    $field_tags_handler_settings = [
      'target_bundles' => [
        'tracked_keyword_vocabulary',
      ],
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
   * @return \Drupal\user\UserInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private function getUser($username = 'a_username') {
    $user = $this->getMock('Drupal\user\UserInterface');
    $user->expects($this->once())
      ->method('getUsername')
      ->willReturn($username);
    return $user;
  }
  /**
   * testPopulateHtmlHeadWithNodeAndFields(), sub routine "setup fields".
   */
  private function populateHtmlHeadWithNodeAndFieldsSetUpFields() {
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
   * To render array.
   *
   * @param array $pageContextConfig
   *   The page context config
   * @param string $assetsUrl
   *   The assets URL
   * @return array
   *   The render array
   */
  private function toRenderArray($pageContextConfig, $assetsUrl) {
    $renderArray = ['old_head'];

    foreach ($pageContextConfig as $name => $content) {
      $renderArray[] = [
        [
          '#type' => 'html_tag',
          '#tag' => 'meta',
          '#attributes' => [
            'itemprop' => 'acquia_lift:' . $name,
            'content' => $content,
          ],
        ],
        $name,
      ];
    }

    $renderArray[] = [
      [
        '#tag' => 'script',
        '#attributes' => [
          'src' => $assetsUrl . '/lift.js',
        ],
      ],
      'acquia_lift_javascript',
    ];

    return $renderArray;
  }
}
