<?php

namespace Drupal\Tests\acquia_lift\Unit\Service\Context;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Context\PageContext;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;

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
   * Language manager interface.
   *
   * @var Drupal\Core\Language\LanguageManagerInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $language;

  /**
   * Language Interface.
   * 
   * @var Drupal\Core\Language\LanguageInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $languageInterface;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    // Get config factory mock
    $this->configFactory = $this->getMock('Drupal\Core\Config\ConfigFactoryInterface');

    // Get settings mock
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\ImmutableConfig')
      ->disableOriginalConstructor()
      ->getMock();

    // Get entity manager mock
    $this->entityTypeManager = $this->getMock('Drupal\Core\Entity\EntityTypeManagerInterface');

    // Get taxonomy term mock
    $this->taxonomyTermStorage = $this->getMock('Drupal\taxonomy\TermStorageInterface');

    // Get request class mocks
    $this->requestStack = $this->getMock('Symfony\Component\HttpFoundation\RequestStack');
    $this->request = $this->getMock('Symfony\Component\HttpFoundation\Request');
    $this->requestParameterBag = $this->getMock('Symfony\Component\HttpFoundation\ParameterBag');

    // Get route mocks
    $this->routeMatch = $this->getMock('Drupal\Core\Routing\RouteMatchInterface');
    $this->route = $this->getMockBuilder('Symfony\Component\Routing\Route')
      ->disableOriginalConstructor()
      ->getMock();

    // Get title resolver mock
    $this->titleResolver = $this->getMock('Drupal\Core\Controller\TitleResolverInterface');

    // Get language mock
    $this->language = $this->getMock('Drupal\Core\Language\LanguageManagerInterface');

    // Get language object mock
    $this->languageInterface = $this->getMock('Drupal\Core\Language\LanguageInterface');

    // Mock method and return val
    $this->languageInterface
      ->expects($this->any())
      ->method('getId')
      ->willReturn('fr');
    
    // Mock config factory
    $this->configFactory->expects($this->once())
      ->method('get')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);

    // Mock settings credential method and return val
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($this->getValidCredentialSettings());

    // Mock settings field_mapping method and return val
    $this->settings->expects($this->at(1))
      ->method('get')
      ->with('field_mappings')
      ->willReturn($this->getValidFieldMappingsSettings());

    // Mock settings udf_person_mappings method and return val
    $this->settings->expects($this->at(2))
      ->method('get')
      ->with('udf_person_mappings')
      ->willReturn($this->getValidUdfPersonMappingsSettings());

    // Mock settings udf_touch_mappings method and return val
    $this->settings->expects($this->at(3))
      ->method('get')
      ->with('udf_touch_mappings')
      ->willReturn($this->getValidUdfTouchMappingsSettings());

    // Mock settings udf_event_mappings method and return val
    $this->settings->expects($this->at(4))
      ->method('get')
      ->with('udf_event_mappings')
      ->willReturn($this->getValidUdfEventMappingsSettings());

    // Mock settings advanced method and return val
    $this->settings->expects($this->at(5))
      ->method('get')
      ->with('advanced')
      ->willReturn($this->getValidAdvancedSettings());

    // Mock entity type manager getStorage method and return val
    $this->entityTypeManager->expects($this->once())
      ->method('getStorage')
      ->with('taxonomy_term')
      ->willReturn($this->taxonomyTermStorage);

    // Mock request stack's getCurrentRequest method and return val
    $this->requestStack->expects($this->once())
      ->method('getCurrentRequest')
      ->willReturn($this->request);

    // Set param bag
    $this->request->attributes = $this->requestParameterBag;

    // Mock routeMatch getRouteObject method and return val
    $this->routeMatch->expects($this->once())
      ->method('getRouteObject')
      ->willReturn($this->route);
  }

  /**
   * Tests the populate() method, populateHtmlHead() sub method, credential configuration.
   *
   * @covers ::populate
   */
  public function testPopulateHtmlHeadCredentialConfiguration() {
    $this->requestParameterBag->expects($this->once())
      ->method('has')
      ->with('node')
      ->willReturn(FALSE);

    // Language mock
    $this->language
      ->expects($this->any())
      ->method('getCurrentLanguage')
      ->willReturn($this->languageInterface);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver, $this->language);
    $page = [];
    $page_context->populate($page);

    $expected_head = $this->toRenderArray([
      'content_title' => 'Untitled',
      'content_type' => 'page',
      'page_type' => 'content page',
      'context_language' => 'fr',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '',
      'content_uuid' => '',
      'published_date' => '',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'account_id' => 'AccountId1',
      'site_id' => 'SiteId1',
      'userAccess' => 'user_access_1',
      'liftAssetsURL' => 'AssetsUrl1',
      'liftDecisionAPIURL' => 'decision_api_url_1',
      'bootstrapMode' => 'manual',
      'contentReplacementMode' => 'customized',
      'cdfVersion' => 2,
    ], 'AssetsUrl1');

    $this->assertEquals($expected_head, $page['#attached']['html_head']);
  }

  /**
   * Tests the populate method, populateHtmlHead() sub method, with a Node.
   *
   * @covers ::populate
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

    // Language mock
    $this->language
      ->expects($this->any())
      ->method('getCurrentLanguage')
      ->willReturn($this->languageInterface);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver, $this->language);
    $page = [];
    $page_context->populate($page);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title',
      'content_type' => 'article',
      'page_type' => 'node page',
      'context_language' => 'fr',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '90210',
      'content_uuid' => 'ecf826eb-3ef0-4aa6-aae2-9f6e5886bbb6',
      'published_date' => 'a_published_time',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'account_id' => 'AccountId1',
      'site_id' => 'SiteId1',
      'userAccess' => 'user_access_1',
      'liftAssetsURL' => 'AssetsUrl1',
      'liftDecisionAPIURL' => 'decision_api_url_1',
      'bootstrapMode' => 'manual',
      'contentReplacementMode' => 'customized',
      'cdfVersion' => 2,
    ], 'AssetsUrl1');

    $this->assertEquals($expected_head, $page['#attached']['html_head']);
  }

  /**
   * Tests the populate() method, populateHtmlHead() sub method, with a Node and simple title.
   *
   * @covers ::populate
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

    // Language mock
    $this->language
      ->expects($this->any())
      ->method('getCurrentLanguage')
      ->willReturn($this->languageInterface);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver, $this->language);
    $page = [];
    $page_context->populate($page);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title from Title Resolver',
      'content_type' => 'article',
      'page_type' => 'node page',
      'context_language' => 'fr',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '90210',
      'content_uuid' => 'ecf826eb-3ef0-4aa6-aae2-9f6e5886bbb6',
      'published_date' => 'a_published_time',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'account_id' => 'AccountId1',
      'site_id' => 'SiteId1',
      'userAccess' => 'user_access_1',
      'liftAssetsURL' => 'AssetsUrl1',
      'liftDecisionAPIURL' => 'decision_api_url_1',
      'bootstrapMode' => 'manual',
      'contentReplacementMode' => 'customized',
      'cdfVersion' => 2,
    ], 'AssetsUrl1');

    $this->assertEquals($expected_head, $page['#attached']['html_head']);
  }

  /**
   * Tests the populate() method, populateHtmlHead() sub method, with a Node and array title.
   *
   * @covers ::populate
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


    // Language mock
    $this->language
      ->expects($this->any())
      ->method('getCurrentLanguage')
      ->willReturn($this->languageInterface);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver, $this->language);
    $page = [];
    $page_context->populate($page);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title from Title Resolver <br />',
      'content_type' => 'article',
      'page_type' => 'node page',
      'context_language' => 'fr',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '90210',
      'content_uuid' => 'ecf826eb-3ef0-4aa6-aae2-9f6e5886bbb6',
      'published_date' => 'a_published_time',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'account_id' => 'AccountId1',
      'site_id' => 'SiteId1',
      'userAccess' => 'user_access_1',
      'liftAssetsURL' => 'AssetsUrl1',
      'liftDecisionAPIURL' => 'decision_api_url_1',
      'bootstrapMode' => 'manual',
      'contentReplacementMode' => 'customized',
      'cdfVersion' => 2,
    ], 'AssetsUrl1');

    $this->assertEquals($expected_head, $page['#attached']['html_head']);
  }

  /**
   * Tests the populate() method, populateHtmlHead() sub method, with a Node and fields.
   *
   * @covers ::populate
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

    // Language mock
    $this->language
      ->expects($this->any())
      ->method('getCurrentLanguage')
      ->willReturn($this->languageInterface);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager, $this->requestStack, $this->routeMatch, $this->titleResolver, $this->language);
    $page = [];
    $page_context->populate($page);

    $expected_head = $this->toRenderArray([
      'content_title' => 'My Title',
      'content_type' => 'article',
      'page_type' => 'node page',
      'context_language' => 'fr',
      'content_section' => 'Tracked Content Term Name 1',
      'content_keywords' => 'Tracked Keyword Term Name 1,Tracked Keyword Term Name 2',
      'post_id' => '90210',
      'content_uuid' => 'ecf826eb-3ef0-4aa6-aae2-9f6e5886bbb6',
      'published_date' => 'a_published_time',
      'persona' => '',
      'engagement_score' => PageContext::ENGAGEMENT_SCORE_DEFAULT,
      'account_id' => 'AccountId1',
      'site_id' => 'SiteId1',
      'userAccess' => 'user_access_1',
      'liftAssetsURL' => 'AssetsUrl1',
      'liftDecisionAPIURL' => 'decision_api_url_1',
      'bootstrapMode' => 'manual',
      'contentReplacementMode' => 'customized',
      'cdfVersion' => 2,
      'event_udf1' => 'Tracked Content Term Name 1',
      'touch_udf1' => 'Tracked Content Term Name 1',
      'person_udf1' => 'Tracked Keyword Term Name 1,Tracked Keyword Term Name 2',
      'event_udf2' => 'Tracked Keyword Term Name 1,Tracked Keyword Term Name 2',
    ], 'AssetsUrl1');

    $this->assertEquals($expected_head, $page['#attached']['html_head']);
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
      ->method('bundle')
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

    $node->expects($this->once())
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
      ->method('uuid')
      ->willReturn('ecf826eb-3ef0-4aa6-aae2-9f6e5886bbb6');

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
    $renderArray = [];

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
          'async' => true,
        ],
      ],
      'acquia_lift_javascript',
    ];

    return $renderArray;
  }
}
