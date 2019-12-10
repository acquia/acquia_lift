<?php

namespace Drupal\acquia_lift_publisher\EventSubscriber\Cdf;

use Acquia\ContentHubClient\CDFAttribute;
use Acquia\ContentHubClient\CDF\CDFObject;
use Drupal\acquia_contenthub\AcquiaContentHubEvents;
use Drupal\acquia_contenthub\Client\ClientFactory;
use Drupal\acquia_contenthub\Event\CreateCdfEntityEvent;
use Drupal\acquia_contenthub\Session\ContentHubUserSession;
use Drupal\Component\Uuid\UuidInterface;
use Drupal\Core\Block\BlockManagerInterface;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Render\Element;
use Drupal\Core\Render\RendererInterface;
use Drupal\Core\Session\AccountSwitcherInterface;
use Drupal\image\Entity\ImageStyle;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

/**
 * Class EntityRenderHandler.
 *
 * @package Drupal\acquia_lift_publisher\EventSubscriber\Cdf
 */
class EntityRenderHandler implements EventSubscriberInterface {

  /**
   * The account switcher.
   *
   * @var \Drupal\Core\Session\AccountSwitcherInterface
   */
  protected $accountSwitcher;

  /**
   * The Acquia Lift entity configuration.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   */
  protected $config;

  /**
   * The origin uuid.
   *
   * @var string
   */
  protected $origin;

  /**
   * The renderer.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The Block Manager.
   *
   * @var \Drupal\Core\Block\BlockManagerInterface
   */
  protected $blockManager;

  /**
   * The uuid generator.
   *
   * @var \Drupal\Component\Uuid\UuidInterface
   */
  protected $uuidGenerator;

  /**
   * The rendered user.
   *
   * @var \Drupal\Core\Session\UserSession
   */
  protected $renderUser;

  /**
   * The storage.
   *
   * @var array
   */
  protected $storage = [];

  /**
   * The client factory.
   *
   * @var \Drupal\acquia_contenthub\Client\ClientFactory
   */
  protected $clientFactory;

  /**
   * EntityRenderHandler constructor.
   *
   * @param \Drupal\Core\Session\AccountSwitcherInterface $account_switcher
   *   The account switcher.
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory.
   * @param \Drupal\Core\Render\RendererInterface $renderer
   *   The renderer.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Block\BlockManagerInterface $block_manager
   *   The block manager.
   * @param \Drupal\Component\Uuid\UuidInterface $uuid_generator
   *   The UUID generator.
   * @param \Drupal\acquia_contenthub\Client\ClientFactory $client_factory
   *   The client factory.
   */
  public function __construct(AccountSwitcherInterface $account_switcher, ConfigFactoryInterface $config_factory, RendererInterface $renderer, EntityTypeManagerInterface $entity_type_manager, BlockManagerInterface $block_manager, UuidInterface $uuid_generator, ClientFactory $client_factory) {
    $this->accountSwitcher = $account_switcher;
    $this->config = $config_factory->get('acquia_lift_publisher.entity_config');
    $this->clientFactory = $client_factory;
    $this->origin = $client_factory->getSettings()->getUuid();
    $this->renderer = $renderer;
    $this->entityTypeManager = $entity_type_manager;
    $this->blockManager = $block_manager;
    $this->uuidGenerator = $uuid_generator;
  }

  /**
   * {@inheritdoc}
   */
  public static function getSubscribedEvents() {
    $events[AcquiaContentHubEvents::CREATE_CDF_OBJECT][] = ['onCreateCdf', 100];
    return $events;
  }

  /**
   * Actions on create CDF.
   *
   * @param \Drupal\acquia_contenthub\Event\CreateCdfEntityEvent $event
   *   The create CDF entity event.
   *
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function onCreateCdf(CreateCdfEntityEvent $event) {
    $entity = $event->getEntity();
    if (!$entity instanceof ContentEntityInterface) {
      // @todo we should support config entity rendering too.
      return;
    }
    if ($view_modes = $this->config->get("view_modes.{$entity->getEntityTypeId()}.{$entity->bundle()}")) {
      $document = $this->clientFactory->getClient()
        ->getEntities([$entity->uuid()]);
      $remote_entity = $document->hasEntity($entity->uuid()) ? $document->getCDFEntity($entity->uuid()) : FALSE;
      foreach (array_keys($view_modes) as $view_mode) {
        // The preview image field setting is saved along side the view modes.
        // Don't process it as one.
        if ($view_mode == 'acquia_lift_preview_image') {
          continue;
        }
        $display = \Drupal::entityTypeManager()->getStorage('entity_view_mode')->load("{$entity->getEntityTypeId()}.$view_mode");
        foreach ($entity->getTranslationLanguages() as $language) {
          $entity = $entity->getTranslation($language->getId());

          if ($remote_entity instanceof CDFObject) {
            $uuid = $this->getUuid($remote_entity, $view_mode, $language->getId());
          }
          else {
            $uuid = $this->uuidGenerator->generate();
          }
          $cdf = new CDFObject('rendered_entity', $uuid, date('c'), date('c'), $this->origin);
          $elements = $this->getViewModeMinimalHtml($entity, $view_mode);
          $html = $this->renderer->renderPlain($elements);
          $metadata['data'] = base64_encode($html);
          $cdf->addAttribute('content', CDFAttribute::TYPE_STRING, trim(preg_replace('/\s+/', ' ', str_replace("\n", ' ', strip_tags($html)))));
          $cdf->addAttribute('source_entity', CDFAttribute::TYPE_STRING, $entity->uuid());
          $cdf->addAttribute('label', CDFAttribute::TYPE_ARRAY_STRING, $entity->label(), $entity->language()->getId());
          $cdf->addAttribute('language', CDFAttribute::TYPE_STRING, $language->getId());
          $cdf->addAttribute('language_label', CDFAttribute::TYPE_STRING, $language->getName());
          $cdf->addAttribute('view_mode', CDFAttribute::TYPE_STRING, $view_mode);
          $cdf->addAttribute('view_mode_label', CDFAttribute::TYPE_STRING, $display->label());

          if (isset($view_modes['acquia_lift_preview_image'])) {
            $preview_image = $entity->{$view_modes['acquia_lift_preview_image']}->first();

            if (!$preview_image) {
              continue;
            }

            $src = ImageStyle::load('acquia_lift_publisher_preview_image')->buildUrl($preview_image->entity->getFileUri());
            $cdf->addAttribute('preview_image', CDFAttribute::TYPE_STRING, $src);
          }

          $cdf->setMetadata($metadata);
          $event->addCdf($cdf);
        }
      }
    }
  }

  /**
   * Get rendered user.
   *
   * @return \Drupal\acquia_contenthub\Session\ContentHubUserSession|\Drupal\Core\Session\UserSession
   *   The rendered user.
   */
  protected function getRenderUser() {
    if (!$this->renderUser) {
      $this->renderUser = new ContentHubUserSession($this->config->get('render_role'));
    }
    return $this->renderUser;
  }

  /**
   * Get view mode minimal HTML.
   *
   * @param \Drupal\Core\Entity\ContentEntityInterface $object
   *   The content entity object.
   * @param string $view_mode
   *   The view mode identifier.
   *
   * @return array
   *   The view mode minimal HTML.
   */
  protected function getViewModeMinimalHtml(ContentEntityInterface $object, $view_mode) {
    // Creating a fake user account to give as context to the normalization.
    $account = $this->getRenderUser();
    // Checking for entity access permission to this particular account.
    $entity_access = $object->access('view', $account, TRUE);
    if (!$entity_access->isAllowed()) {
      return [];
    }
    $this->accountSwitcher->switchTo($account);
    // Render View Mode.
    $entity_type_id = $object->getEntityTypeId();
    // @todo allow different entity types to specify how to do this.
    if ($entity_type_id === 'block_content') {
      $build = $this->getBlockMinimalBuildArray($object, $view_mode);
    }
    else {
      $build = $this->entityTypeManager->getViewBuilder($entity_type_id)
        ->view($object, $view_mode, $object->language()->getId());
    }
    // Restore user account.
    $this->accountSwitcher->switchBack();
    return $build;
  }

  /**
   * Renders block using BlockViewBuilder.
   *
   * @param \Drupal\Core\Entity\ContentEntityInterface $object
   *   The Content Entity Object.
   * @param string $view_mode
   *   The request view mode identifier.
   *
   * @return array
   *   Render array for the block.
   */
  protected function getBlockMinimalBuildArray(ContentEntityInterface $object, $view_mode) {
    /** @var \Drupal\block_content\Plugin\Block\BlockContentBlock $block */
    $block = $this->blockManager->createInstance('block_content:' . $object->uuid());
    $build = [
      '#theme' => 'block',
      '#attributes' => [],
      '#contextual_links' => [],
      '#weight' => 0,
      '#configuration' => $block->getConfiguration(),
      '#plugin_id' => $block->getPluginId(),
      '#base_plugin_id' => $block->getBaseId(),
      '#derivative_plugin_id' => $block->getDerivativeId(),
    ];
    // Label controlled by the block__block_content__acquia_contenthub template
    // (hidden by default). Override the template in your theme to render a
    // block label.
    if ($build['#configuration']['label'] === '') {
      $build['#configuration']['label'] = $block->label();
    }
    // Block entity itself doesn't have configuration.
    $block->setConfigurationValue('view_mode', $view_mode);
    $build['#configuration']['view_mode'] = $view_mode;
    // See \Drupal\block\BlockViewBuilder::preRender() for reference.
    $content = $block->build();
    if ($content !== NULL && !Element::isEmpty($content)) {
      foreach (['#attributes', '#contextual_links'] as $property) {
        if (isset($content[$property])) {
          $build[$property] += $content[$property];
          unset($content[$property]);
        }
      }
    }
    $build['content'] = $content;
    return $build;
  }

  /**
   * Get UUID.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject $source_entity_cdf
   *   The source entity CDF.
   * @param string $view_mode
   *   The view mode identifier.
   * @param string $langcode
   *   The language code.
   *
   * @return mixed
   *   The UUID.
   */
  protected function getUuid(CDFObject $source_entity_cdf, $view_mode, $langcode) {
    $source_uuid = $source_entity_cdf->getUuid();

    if ($this->isStorageHit($source_uuid, $langcode, $view_mode)) {
      return $this->getStorageItem($source_uuid, $langcode, $view_mode);
    }

    if (!$this->storageIsAlreadyWarmedUp($source_uuid)) {
      $response = $this->clientFactory
        ->getClient()
        ->listEntities([
          'type' => 'rendered_entity',
          'origin' => $this->origin,
          'fields' => 'language,view_mode',
          'filters' => [
            'source_entity' => $source_uuid,
          ],
        ]);

      if (TRUE === $response['success'] && !empty($response['data'])) {
        $this->storageWarmUp($response['data'], $source_uuid);
      }
    }

    if (!$this->isStorageHit($source_uuid, $langcode, $view_mode)) {
      $this->setStorageItem($source_uuid, $langcode, $view_mode, $this->uuidGenerator->generate());
    }

    return $this->getStorageItem($source_uuid, $langcode, $view_mode);
  }

  /**
   * Get attribute value.
   *
   * @param array $attribute
   *   The attribute.
   * @param string $key
   *   The key.
   * @param string|null $default
   *   The default.
   *
   * @return mixed|null
   *   The und value or the default value.
   */
  protected function getAttributeValue($attribute, $key, $default = NULL) {
    return $attribute[$key]['und'] ?? $default;
  }

  /**
   * Check if the storage is a hit.
   *
   * @param string $uuid
   *   The UUID.
   * @param string $langcode
   *   The language code.
   * @param string $view_mode
   *   The view mode identifier.
   *
   * @return bool
   *   TRUE if storage hit; FALSE otherwise.
   */
  protected function isStorageHit($uuid, $langcode, $view_mode) {
    return isset($this->storage[$uuid][$langcode][$view_mode]);
  }

  /**
   * Get storage item.
   *
   * @param string $uuid
   *   The UUID.
   * @param string $langcode
   *   The language code.
   * @param string $view_mode
   *   The view mode identifier.
   *
   * @return mixed
   *   The value.
   */
  protected function getStorageItem($uuid, $langcode, $view_mode) {
    return $this->storage[$uuid][$langcode][$view_mode];
  }

  /**
   * Set storage item.
   *
   * @param string $uuid
   *   The UUID.
   * @param string $langcode
   *   The language code.
   * @param string $view_mode
   *   The view mode identifier.
   * @param mixed $value
   *   The value.
   */
  protected function setStorageItem($uuid, $langcode, $view_mode, $value) {
    $this->storage[$uuid][$langcode][$view_mode] = $value;
  }

  /**
   * Check if the storage is already warmed up.
   *
   * @param string $uuid
   *   The UUID.
   *
   * @return bool
   *   TRUE if the storage is already warmed up; FALSE otherwise.
   */
  protected function storageIsAlreadyWarmedUp($uuid) {
    return isset($this->storage[$uuid]);
  }

  /**
   * Storage warm up action.
   *
   * @param array $data
   *   The array of data.
   * @param string $source_uuid
   *   The source UUID.
   */
  protected function storageWarmUp(array $data, $source_uuid) {
    foreach ($data as $entity_info) {
      $this->setStorageItem(
        $source_uuid,
        $this->getAttributeValue($entity_info['attributes'], 'language'),
        $this->getAttributeValue($entity_info['attributes'], 'view_mode'),
        $entity_info['uuid']
      );
    }
  }

}
