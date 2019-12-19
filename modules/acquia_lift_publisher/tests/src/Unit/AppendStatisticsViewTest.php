<?php

namespace Drupal\Tests\acquia_lift_publisher\Unit;

use Acquia\ContentHubClient\CDF\CDFObject;
use Drupal\acquia_contenthub\Event\CdfAttributesEvent;
use Drupal\acquia_lift_publisher\EventSubscriber\Cdf\AppendStatisticsView;
use Drupal\Core\Config\Entity\ConfigEntityInterface;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\node\NodeInterface;
use Drupal\Tests\UnitTestCase;

/**
 * Class AppendStatisticsViewTest.
 *
 * @coversDefaultClass \Drupal\acquia_lift_publisher\EventSubscriber\Cdf\AppendStatisticsView
 *
 * @group acquia_lift_publisher
 */
class AppendStatisticsViewTest extends UnitTestCase {

  /**
   * @var \Drupal\acquia_lift_publisher\EventSubscriber\Cdf\AppendStatisticsView
   */
  private $subscriber;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->subscriber = new AppendStatisticsView();
  }

  /**
   * Tests if metadata has the necessary field.
   *
   * @param array $cdf_array
   *   The CDF constructing array.
   * @param string $entity
   *   The entity to prophesize.
   * @param string $expected
   *   The expected encoded CDF data.
   *
   * @covers ::onPopulateAttributes
   * @dataProvider cdfDataProvider
   *
   * @throws \ReflectionException
   */
  public function testOnPopulateAttributes(array $cdf_array, string $entity, string $expected) {
    $cdf = CDFObject::fromArray($cdf_array);
    $entity_mock = $this->prophesize($entity);
    $event = new CdfAttributesEvent($cdf, $entity_mock->reveal());
    $this->subscriber->onPopulateAttributes($event);
    $altered = $event->getCdf()->getMetadata()['data'];

    $this->assertEquals($altered, $expected, 'Data matches the expected value.');
  }

  /**
   * Provides sample CDFs.
   *
   * @return array
   *   The array of test cases.
   */
  public function cdfDataProvider(): array {
    $time = time();
    // Default data used in test cases.
    $common_data = [
      'type' => 'drupal8_content_entity',
      'uuid' => 'dbd533f6-1cb5-4b23-b4bb-0ee75408bcc7',
      'created' => $time,
      'modified' => $time,
      'origin' => 'dbd533f6-1cb5-4b23-b4bb-0ee75408bcc8',
      'attributes' => [],
      'metadata' => [
        'data' => [
          'body' => [
            'field_type' => 'text_with_summary',
            'value' => [
              'en' => [
                'value' => [
                  'test body',
                ],
              ],
            ],
          ],
        ],
      ],
    ];

    // Expected data value under metadata.
    $expected = [
      'body' => [
        'field_type' => 'text_with_summary',
        'value' => [
          'en' => [
            'value' => [
              'test body',
            ],
          ],
        ],
      ],
    ];

    // Add previously modified statistics view field.
    $data1 = $common_data;
    $data1['metadata']['data'] = $this->encode(
      $this->appendStatisticsView($data1['metadata']['data'], '15')
    );

    // Encode remaining data to base64.
    $common_data['metadata']['data'] = $this->encode($common_data['metadata']['data']);
    $encoded = $this->encode($expected);

    return [
      [$common_data, ContentEntityInterface::class, $encoded],
      [$common_data, ConfigEntityInterface::class, $encoded],
      [
        $common_data,
        NodeInterface::class,
        $this->encode($this->appendStatisticsView($expected)),
      ],
      // Data should not be overridden.
      [
        $data1,
        NodeInterface::class,
        $this->encode($this->appendStatisticsView($expected, '15')),
      ],
    ];
  }

  /**
   * Appends statistics field to the array.
   *
   * @param array $data
   *   The data to alter.
   * @param string $value
   *   The value of views.
   *
   * @return array
   *   The altered array.
   */
  private function appendStatisticsView(array $data, string $value = '0'): array {
    $data['statistics'] = ['views' => $value];
    return $data;
  }

  /**
   * Encodes to json then with base64.
   *
   * @param array $data
   *   The data to encode.
   *
   * @return string
   *   The base64 string.
   */
  private function encode(array $data): string {
    return base64_encode(json_encode($data));
  }

}
