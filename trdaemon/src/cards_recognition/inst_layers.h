/**
	Purpose:
		To define new layers for using in convolutional network you should extern
		instantiate layer

			extern INSTANTIATE_CLASS(%LayerName%Layer);

		If layer hadn't instantiated you should open /caffe-windows/build_cpu_only/MainBuilder.sln and choose %LayerName%.cpp file in caffelib. 
		Then write 
			REGISTER_LAYER_CLASS(%LayerName%);

		And rebuild caffelib.


	@author Domnich A.
*/


#include "caffe\layers\memory_data_layer.hpp"
#include "caffe\layers\conv_layer.hpp"
#include "caffe\layers\relu_layer.hpp"
#include "caffe\layers\prelu_layer.hpp"
#include "caffe\layers\pooling_layer.hpp"
#include "caffe\layers\inner_product_layer.hpp"
#include "caffe\layers\softmax_layer.hpp"
#include "caffe\layers\dropout_layer.hpp"


namespace caffe {
	extern INSTANTIATE_CLASS(MemoryDataLayer);
	extern INSTANTIATE_CLASS(ConvolutionLayer);
	extern INSTANTIATE_CLASS(ReLULayer);
	extern INSTANTIATE_CLASS(PReLULayer);
	extern INSTANTIATE_CLASS(PoolingLayer);
	extern INSTANTIATE_CLASS(InnerProductLayer);
	extern INSTANTIATE_CLASS(SoftmaxLayer);
	extern INSTANTIATE_CLASS(DropoutLayer);

}