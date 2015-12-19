<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\Service\Context\PageContextTest.
 */

namespace Drupal\acquia_lift\Tests\Service\Context;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Context\PageContext;
use Drupal\acquia_lift\Tests\Traits\SettingsDataTrait;

require_once(__DIR__ . '/../../../Traits/SettingsDataTrait.php');

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
      ->with('field_mappings')
      ->willReturn($this->getValidFieldMappingsSettings());
    $this->settings->expects($this->at(1))
      ->method('get')
      ->with('thumbnail')
      ->willReturn($this->getValidThumbnailSettings());
    $this->entityTypeManager->expects($this->once())
      ->method('getStorage')
      ->with('taxonomy_term')
      ->willReturn($this->taxonomyTermStorage);
  }

  /**
   * Tests the getAll() method, without setNode().
   *
   * @covers ::getAll
   */
  public function testGetAllWithoutSetNode() {
    $page_context = new PageContext($this->configFactory, $this->entityTypeManager);
    $all_page_context = $page_context->getAll();
    $expected_page_context = [
      'content_title' => 'Untitled',
      'content_type' => 'page',
      'page_type' => 'content page',
      'content_section' => '',
      'content_keywords' => '',
      'post_id' => '',
      'published_date' => '',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => 1,
      'author' => '',
      'evalSegments' => TRUE,
      'trackingId' => '',
    ];

    $this->assertEquals($expected_page_context, $all_page_context);
  }

  /**
   * Tests the getAll() method, with setNode().
   *
   * @covers ::getAll
   */
  public function testGetAllWithSetNode() {
    $node = $this->getNode();
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
      ]
    ];
    $this->taxonomyTermStorage->expects($this->once())
      ->method('getNodeTerms')
      ->with([90210])
      ->willReturn($terms);

    $page_context = new PageContext($this->configFactory, $this->entityTypeManager);
    $page_context->set($node);
    $all_page_context = $page_context->getAll();
    $expected_page_context = [
      'content_title' => 'My Title',
      'content_type' => 'article',
      'page_type' => 'node page',
      'content_section' => 'Tracked Content Term Name 1',
      'content_keywords' => 'Tracked Keyword Term Name 1,Tracked Keyword Term Name 2',
      'post_id' => 90210,
      'published_date' => 'a_published_time',
      'thumbnail_url' => '',
      'persona' => '',
      'engagement_score' => 1,
      'author' => 'a_username',
      'evalSegments' => TRUE,
      'trackingId' => '',
    ];

    $this->assertEquals($expected_page_context, $all_page_context);
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
}
