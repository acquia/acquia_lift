<?php

namespace Drupal\acquia_lift_publisher\EventSubscriber\Cdf;

use Drupal\acquia_contenthub\AcquiaContentHubEvents;
use Drupal\acquia_contenthub\Event\CdfAttributesEvent;
use Drupal\Core\Config\Entity\ConfigEntityInterface;
use Drupal\Core\Entity\ContentEntityInterface;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

/**
 * Class AppendStatisticsView
 *
 * @package Drupal\acquia_lift_publisher\EventSubscriber\Cdf
 */
class AppendStatisticsView implements EventSubscriberInterface {

  /**
   * {@inheritdoc}
   */
  public static function getSubscribedEvents() {
    $events[AcquiaContentHubEvents::POPULATE_CDF_ATTRIBUTES][] = ['onPopulateAttributes', 150];
    return $events;
  }

  /**
   * Appends statistics.views field with a default value.
   */
  public function onPopulateAttributes(CdfAttributesEvent $event) {
    $entity = $event->getEntity();
    if (!$entity instanceof ContentEntityInterface) {
      return;
    }

    $cdf = $event->getCdf();
    $metadata = $cdf->getMetadata();
    if (!isset($metadata['data'])) {
      return;
    }

    $append_val = [
      'statistics' => [
        'views' => '0',
      ],
    ];

    $data = json_decode(base64_decode($metadata['data']), TRUE);
    if (isset($data['statistics'])) {
      return;
    }
    $data = array_merge($data, $append_val);
    $metadata['data'] = base64_encode(json_encode($data));
    $cdf->setMetadata($metadata);
  }

}
