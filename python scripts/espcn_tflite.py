#%%
import tensorflow as tf

import os
import math
import numpy as np

from tensorflow import keras
from tensorflow.keras import layers
from tensorflow.keras.preprocessing.image import load_img
from tensorflow.keras.preprocessing.image import array_to_img
from tensorflow.keras.preprocessing.image import img_to_array
from tensorflow.keras.preprocessing import image_dataset_from_directory

from IPython.display import display
assert float(tf.__version__[:3]) >= 2.3

#%%
interpreter = tf.lite.Interpreter(model_path="C:/Users/ivmu6294/AndroidApps/hello-gl2/app/src/main/assets/model_from_h5_edge.tflite")
interpreter.allocate_tensors()

#Get input and output tensors.
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
print(f"Input shape: {input_details[0]['shape']}, dtype: {input_details[0]['dtype']}")
print(f"Output shape: {output_details[0]['shape']}, dtype: {output_details[0]['dtype']}")

interpreter.resize_tensor_input(0, [1, 640, 360, 1])
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
print(f"Input shape: {input_details[0]['shape']}, dtype: {input_details[0]['dtype']}")
print(f"Output shape: {output_details[0]['shape']}, dtype: {output_details[0]['dtype']}")

#%%
def representative_dataset():
    for _ in range(100):
        data = np.random.rand(1, 360, 800, 1)
        yield [data.astype(np.float32)]
#%%
print(tf.version.VERSION)
model = tf.keras.models.load_model('C:/Users/ivmu6294/Desktop/Masterarbeit/h5_model.h5')
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8
tflite_model_edge = converter.convert()



# %%
interpreter = tf.lite.Interpreter(model_content=tflite_model_edge)
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
print(f"Input shape: {input_details[0]['shape']}, dtype: {input_details[0]['dtype']}")
print(f"Output shape: {output_details[0]['shape']}, dtype: {output_details[0]['dtype']}")

interpreter.resize_tensor_input(0, [1, 360, 800, 1])
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
print(f"Input shape: {input_details[0]['shape']}, dtype: {input_details[0]['dtype']}")
print(f"Output shape: {output_details[0]['shape']}, dtype: {output_details[0]['dtype']}")

#%%
with open('model_from_h5_edge.tflite', 'wb') as f:
  f.write(tflite_model_edge)
# %%
tf.lite.experimental.Analyzer.analyze(model_content=tflite_model_edge, gpu_compatibility=True)
# %%
