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
  if ($data['RESULT'] != 0) {
    // @todo Add message.
    drupal_set_message(t('Credit card verification failed: @error.', array('@error' => $data['DESCRIPTION'])), 'error');
    drupal_goto(commerce_checkout_order_uri($order));
  }

  // Store the card ref id in the order object.
  // @todo Add commerce_cardonfile integration.
  $order->data['commerce_saferpay_card_ref_id'] = (string)$data['CARDREFID'];
  commerce_order_save($order);

  $payment_method = commerce_payment_method_instance_load($order->data['payment_method']);
  if (empty($payment_method['settings']['use_mpi'])) {
    // Authorize and optionally settle the order immediatly.
    commerce_saferpay_professional_pay_order($order);
  }
  else {
    $mpi_response = commerce_saferpay_professional_verify_enrollment($order, $data, $payment_method['settings']);
    if ($mpi_response !== FALSE) {
      // Redirect to 3-D secure, if necessary.
      if ((int)$mpi_response['RESULT'] == 0 && (int)$mpi_response['ECI'] == COMMERCE_SAFERPAY_ECI_3D_AUTHENTICATION) {
        header('Location: ' . $mpi_response['MPI_PA_LINK'], TRUE, 302);
        drupal_exit($mpi_response['MPI_PA_LINK']);
      }
      else {
        // Check if there is no liability shift and if such payments are allowed.
        if (((int) $mpi_response['RESULT'] != 0 || $mpi_response['ECI'] == COMMERCE_SAFERPAY_ECI_NO_LIABILITYSHIFT) && !empty($payment_method['settings']['require_liablityshift'])) {
          drupal_set_message(t('Payments from credit cards without 3-D Secure support are not accepted.'), 'error');
          drupal_goto(commerce_checkout_order_uri($order));
        }
        // Authorize and optionally settle the order immediatly.
        commerce_saferpay_professional_pay_order($order, $_SESSION['commerce_saferpay_professional']['card_ref_id'], $mpi_response['MPI_SESSIONID']);
      }
    }
    else {
      drupal_set_message(t('Payment failed.'), 'error');
      drupal_goto(commerce_checkout_order_uri($order));
    }
  }
}

/**
 * Page callback for processing the Saferpay MPI response.
 *
 * @param $order
 *   The commerce order object.
 */
function commerce_saferpay_professional_mpi_handle_response($order) {
  $data = simplexml_load_string($_GET['DATA']);
  if ($data['RESULT'] != 0) {
    drupal_set_message(t('Payment failed: @message', array('@message' => $data['MESSAGE'])));
    drupal_goto(commerce_checkout_order_uri($order));
  }
  // Authorize and optionally settle the order.
  $order->data['commerce_saferpay_mpi_session_id'] = (string)$data['MPI_SESSIONID'];
  commerce_saferpay_professional_pay_order($order);
}
