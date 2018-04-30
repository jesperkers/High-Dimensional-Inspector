/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *    its contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLA PEZZOTTI ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLA PEZZOTTI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include "std_hsne_analytics_embedder_qobj.h"
#include "hdi/utils/log_helper_functions.h"

namespace hdi{
    namespace analytics{

        std::shared_ptr<AbstractHSNEEmbedder> StdHSNEEmbedderFactory::createEmbedder(){
            utils::secureLog(_logger,"New StdHSNEEmbedder generated by StdHSNEEmbedderFactory");
            auto ptr = std::shared_ptr<AbstractHSNEEmbedder>(new StdHSNEEmbedder());
            ptr->setLogger(_logger);
            return ptr;
        }



        void StdHSNEEmbedder::initialize(const sparse_scalar_matrix_type& probabilities){
            utils::secureLog(_logger,"StdHSNEEmbedder initialization");
            utils::secureLogValue(_logger,"id scale",std::get<0>(_id));
            utils::secureLogValue(_logger,"id analysis",std::get<1>(_id));

            _tsne.setLogger(_logger);

            double theta = 0;
            if(probabilities.size() < 1000){
                theta = 0;
                _tsne_params._exaggeration_factor = 1;
            }else if(probabilities.size() < 5000){
                theta = (probabilities.size()-1000.)/(5000.-1000.)*0.5;
                _tsne_params._exaggeration_factor = 1+(probabilities.size()-1000.)/(5000.-1000.)*3;
            }else{
                theta = 0.5;
                _tsne_params._exaggeration_factor = 4;
            }
            _tsne.setTheta(theta);
            utils::secureLogValue(_logger,"theta",theta);
            utils::secureLogValue(_logger,"exg",_tsne_params._exaggeration_factor);

            _tsne.initialize(probabilities,&_embedding,_tsne_params);
            _viewer.show();

            _flags.resize(probabilities.size());
            _drawer.initialize(_viewer.context());
            _drawer.setData(_embedding.getContainer().data(), _flags.data(), probabilities.size());
            _drawer.setAlpha(0.7);
            _drawer.setPointSize(5);
            _viewer.addDrawer(&_drawer);
        }

        void StdHSNEEmbedder::onIterate(){
            _tsne.doAnIteration();
            if(_tsne.iteration() < 1000){
                _tsne.doAnIteration();
                {//limits
                    std::vector<scalar_type> limits;
                    _embedding.computeEmbeddingBBox(limits,0.25);
                    auto tr = QVector2D(limits[1],limits[3]);
                    auto bl = QVector2D(limits[0],limits[2]);
                    _viewer.setTopRightCoordinates(tr);
                    _viewer.setBottomLeftCoordinates(bl);
                }
            }
            _viewer.updateGL();
        }

#if 0
        StdHSNEEmbedder::StdHSNEEmbedder():
            _logger(nullptr),
            _initialized(false),
            _viewer(new hdi::viz::ScatterplotCanvas),
            _selection_controller(new hdi::viz::ControllerSelectionEmbedding),
            _visualization_mode(VisualizationModes::UserDefined)
        {
            connect(_viewer.get(),&viz::ScatterplotCanvas::sgnKeyPressed,this,&StdHSNEEmbedder::onKeyPressedOnCanvas);
            _viewer->resize(800,800);
        }

        void StdHSNEEmbedder::initialize(sparse_scalar_matrix_type& sparse_matrix, id_type my_id, tsne_type::Parameters params){
            checkAndThrowLogic(!_initialized, "Aggregator must be initialized ");
            checkAndThrowLogic(sparse_matrix.size() == _panel_data.numDataPoints(), "Panel data and sparse matrix size must agree");
            utils::secureLogValue(_logger,"Sparse matrix size",sparse_matrix.size());
            _selection_controller->setLogger(_logger);
            _tSNE.setLogger(_logger);

            for(int i = 0; i < sparse_matrix.size(); ++i){ //QUICK FIX!!!!
                if(sparse_matrix[i].size() < 7){
                    //utils::secureLogValue(_logger,"UUUUUUUUUUUUUUUU",sparse_matrix[i].size());
                    int to_add = 7 - sparse_matrix[i].size();
                    for(int v = 0; v < to_add; ++v){
                        int id = rand()%sparse_matrix.size();
                        sparse_matrix[i][id] = 1./to_add;
                    }

                }
            }
            double theta = 0;
            if(sparse_matrix.size() < 1000){
                theta = 0;
                params._exaggeration_factor = 1;
            }else if(sparse_matrix.size() < 5000){
                theta = (sparse_matrix.size()-1000.)/(5000.-1000.)*0.5;
                params._exaggeration_factor = 1+(sparse_matrix.size()-1000.)/(5000.-1000.)*3;
            }else{
                theta = 0.5;
                params._exaggeration_factor = 4;
            }
            _tSNE.setTheta(theta);
            utils::secureLogValue(_logger,"theta",theta);
            utils::secureLogValue(_logger,"exg",params._exaggeration_factor);


            _tSNE.initialize(sparse_matrix,&_embedding,params);

            utils::secureLogValue(_logger,"tSNE theta",theta);

            _selection_controller->setActors(&_panel_data,&_embedding,_viewer.get());
            _selection_controller->initialize();

            //_viewer->setBackgroundColors(qRgb(70,70,70),qRgb(30,30,30));
            _viewer->setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            _viewer->setSelectionColor(qRgb(200,200,200));
            _viewer->show();

            _my_id = my_id;
        }

        void StdHSNEEmbedder::doAnIteration(){
            //if(_tSNE.iteration() < 300){//QUICKPAPER
            if(_tSNE.iteration() < 1000){//QUICKPAPER
                _tSNE.doAnIteration();
                {//limits
                    std::vector<scalar_type> limits;
                    _embedding.computeEmbeddingBBox(limits,0.25);
                    auto tr = QVector2D(limits[1],limits[3]);
                    auto bl = QVector2D(limits[0],limits[2]);
                    _viewer->setTopRightCoordinates(tr);
                    _viewer->setBottomLeftCoordinates(bl);
                }
            }
            _viewer->updateGL();
        }

        void StdHSNEEmbedder::addView(std::shared_ptr<hdi::viz::AbstractView> view){
            _views.push_back(view);
            view->setPanelData(&_panel_data);
            _selection_controller->addView(view.get());
        }
        void StdHSNEEmbedder::addUserDefinedDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer){
            drawer->initialize(_viewer->context());
            _drawers.push_back(drawer);
            onUpdateViewer();
        }
        void StdHSNEEmbedder::addAreaOfInfluenceDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer){
            drawer->initialize(_viewer->context());
            _influence_drawers.push_back(drawer);
            onUpdateViewer();
        }
        void StdHSNEEmbedder::addSelectionDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer){
            drawer->initialize(_viewer->context());
            _selection_drawers.push_back(drawer);
            onUpdateViewer();
        }

        void StdHSNEEmbedder::onKeyPressedOnCanvas(int key){
            utils::secureLogValue(_logger,"Key pressed on canvas",key);
            if(key == Qt::Key_N){
                emit sgnNewAnalysisTriggered(_my_id);
            }
            if(key == Qt::Key_S){
                emit sgnActivateSelectionMode(_my_id);
            }
            if(key == Qt::Key_U){
                emit sgnActivateUserDefinedMode(_my_id);
            }
            if(key == Qt::Key_I){
                emit sgnActivateInfluenceMode(_my_id);
            }
            if(key == Qt::Key_P){
                emit sgnPropagateSelection(_my_id);
            }
            if(key == Qt::Key_C){
                emit sgnClusterizeSelection(_my_id);
            }
        }

        void StdHSNEEmbedder::onActivateUserDefinedMode(){
            _visualization_mode = VisualizationModes::UserDefined;
            onUpdateViewer();
        }

        void StdHSNEEmbedder::onActivateSelectionMode(){
            _visualization_mode = VisualizationModes::Selection;
            onUpdateViewer();
        }

        void StdHSNEEmbedder::onActivateInfluencedMode(){
            _visualization_mode = VisualizationModes::Influence;
            onUpdateViewer();
        }

        void StdHSNEEmbedder::onUpdateViewer(){
            _viewer->removeAllDrawers();
            switch (_visualization_mode) {
            case VisualizationModes::UserDefined:
                for(auto& d: _drawers){
                    _viewer->addDrawer(d.get());
                }
                break;
            case VisualizationModes::Influence:
                for(auto& d: _influence_drawers){
                    _viewer->addDrawer(d.get());
                }
                break;
            case VisualizationModes::Selection:
                for(auto& d: _selection_drawers){
                    _viewer->addDrawer(d.get());
                }
                break;
            default:
                break;
            }
        }
#endif
    }
}