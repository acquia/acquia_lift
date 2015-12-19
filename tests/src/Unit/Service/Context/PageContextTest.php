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
    $term_1 = $this->getTerm('term_1');
    $term_2 = $this->getTerm('term_2');
    $term_3 = $this->getTerm('term_3', 'other_vocabulary_id');
    $terms = [90210 => [$term_1, $term_2, $term_3]];
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
      'content_section' => '',
      'content_keywords' => '',
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
  private function getTerm($name = 'my_term', $vocabulary_id = 'my_vocabulary') {
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
   * @param string $id
   *
   * @return Drupal\node\NodeInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private function getNode($id = 90210) {
    $user = $this->getUser();
    $node = $this->getMock('Drupal\node\NodeInterface');
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
    return $node;
  }

  /**
   * Get User.
   *
   * @param string $id
   *
   * @return Drupal\user\UserInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private function getUser($id = 90210) {
    $user = $this->getMock('Drupal\user\UserInterface');
    $user->expects($this->once())
      ->method('getUsername')
      ->willReturn('a_username');
    return $user;
  }
}
