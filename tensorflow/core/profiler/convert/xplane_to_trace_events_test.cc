/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/core/profiler/convert/xplane_to_trace_events.h"

#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/profiler/utils/xplane_builder.h"
#include "tensorflow/core/profiler/utils/xplane_schema.h"

namespace tensorflow {
namespace profiler {
namespace {

void CreateXSpace(XSpace* space) {
  XPlaneBuilder host_plane(space->add_planes());
  XPlaneBuilder device_plane(space->add_planes());

  host_plane.SetName("cpu");
  host_plane.SetId(0);
  XLineBuilder thread1 = host_plane.GetOrCreateLine(10);
  thread1.SetName("thread1");
  XEventBuilder event1 =
      thread1.AddEvent(*host_plane.GetOrCreateEventMetadata("event1"));
  event1.SetTimestampNs(150000);
  event1.SetDurationNs(10000);
  event1.ParseAndAddStatValue(*host_plane.GetOrCreateStatMetadata("tf_op"),
                              "Relu");
  XLineBuilder thread2 = host_plane.GetOrCreateLine(20);
  thread2.SetName("thread2");
  XEventBuilder event2 =
      thread2.AddEvent(*host_plane.GetOrCreateEventMetadata("event2"));
  event2.SetTimestampNs(160000);
  event2.SetDurationNs(10000);
  event2.ParseAndAddStatValue(*host_plane.GetOrCreateStatMetadata("tf_op"),
                              "Conv2D");

  device_plane.SetName("gpu:0");
  device_plane.SetId(1);
  XLineBuilder stream1 = device_plane.GetOrCreateLine(30);
  stream1.SetName("gpu stream 1");
  XEventBuilder event3 =
      stream1.AddEvent(*device_plane.GetOrCreateEventMetadata("kernel1"));
  event3.SetTimestampNs(180000);
  event3.SetDurationNs(10000);
  event3.ParseAndAddStatValue(
      *device_plane.GetOrCreateStatMetadata("correlation id"), "55");
}

TEST(ConvertXPlaneToTraceEvents, Convert) {
  XSpace xspace;
  CreateXSpace(&xspace);

  Trace trace;
  ConvertXSpaceToTraceEvents(xspace, &trace);

  ASSERT_EQ(trace.devices_size(), 2);
  EXPECT_EQ(trace.devices().at(0).resources_size(), 2);
  EXPECT_EQ(trace.devices().at(1).resources_size(), 1);
  EXPECT_EQ(trace.trace_events_size(), 3);
}

TEST(ConvertXPlaneToTraceEvents, Drop) {
  Trace trace;
  for (int i = 0; i < 100; i++) {
    trace.add_trace_events()->set_timestamp_ps((100 - i) % 50);
  }

  MaybeDropEventsForTraceViewer(&trace, 150);
  EXPECT_EQ(trace.trace_events_size(), 100);  // No dropping.

  MaybeDropEventsForTraceViewer(&trace, 50);
  EXPECT_EQ(trace.trace_events_size(), 50);
  for (const auto& event : trace.trace_events()) {
    EXPECT_LT(event.timestamp_ps(), 25);
  }
}

}  // namespace
}  // namespace profiler
}  // namespace tensorflow
