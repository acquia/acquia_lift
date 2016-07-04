<?php

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
   * Tests the populateHtmlHead() method, minimum.
   *
   * @covers ::populateHtmlHead
   */
  public function testPopulateHtmlHeadMinimum() {
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
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ], 'js_path_1');

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
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ], 'js_path_1');

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
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ], 'js_path_1');

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
      'account_id' => 'account_name_1',
      'site_id' => 'customer_site_1',
      'liftDecisionAPIURL' => 'api_url_1',
      'authEndpoint' => 'oauth_url_1',
    ], 'js_path_1');

    $this->assertEquals($expected_head, $head);
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
   * To render array.
   *
   * @param array $pageContextConfig
   *   The page context config
   * @param string $jsPath
   *   The JavaScript Path
   * @return array
   *   The render array
   */
  private function toRenderArray($pageContextConfig, $jsPath) {
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
          'src' => $jsPath,
        ],
      ],
      'acquia_lift_javascript',
    ];

    return $renderArray;
  }
}
