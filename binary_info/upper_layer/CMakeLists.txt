#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if (use_mqtt)
    add_analytic_directory(mqtt_transport_ul "binary_info/upper_layer")
    add_analytic_directory(mqtt_ws_transport_ul "binary_info/upper_layer")
endif()
if (use_amqp)
    add_analytic_directory(amqp_transport_ul "binary_info/upper_layer")
    add_analytic_directory(amqp_ws_transport_ul "binary_info/upper_layer")
endif()
if (use_http)
    add_analytic_directory(http_transport_ul "binary_info/upper_layer")
endif()

if (${use_prov_client})
    if (use_mqtt)
        add_analytic_directory(prov_mqtt_transport_ul "binary_info/upper_layer")
        add_analytic_directory(prov_mqtt_ws_transport_ul "binary_info/upper_layer")
    endif()
    if (use_amqp)
        add_analytic_directory(prov_amqp_ws_transport_ul "binary_info/upper_layer")
        add_analytic_directory(prov_amqp_transport_ul "binary_info/upper_layer")
    endif()
    if (use_http)
        add_analytic_directory(prov_http_transport_ul "binary_info/upper_layer")
    endif()
endif()
