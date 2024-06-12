/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 460
#extension GL_EXT_ray_tracing : enable

// Define the payload structure for passing data between shaders
struct Payload {
    vec4 hitValue;
};

// Declare rayPayloadInEXT with the Payload structure
layout(location = 0) rayPayloadInEXT Payload payload;

void main()
{
    payload.hitValue = vec4(0.5f, 0.0f, 10.0f, 1.0f);
}