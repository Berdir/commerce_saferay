<?php

/**
 * @file
 * Page callbacks for Saferpay Professional payment gateway.
 */

/**
 * Page callback for processing the Saferpay SCD response.
 *
 * @param $order
 *   The commerce order object.
 */
function commerce_saferpay_professional_scd_handle_response($order) {
  $data = simplexml_load_string($_GET['DATA']);
  if ($data['RESULT'] !== 0) {
    // @todo Add message.
    drupal_goto('checkout/' . $order->order_id . '/payment');
  }
  dpm((array)$data);

  return 'OK';
}
