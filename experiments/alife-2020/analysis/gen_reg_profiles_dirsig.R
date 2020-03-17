library(ggplot2)  # (Wickham, 2009)
library(dplyr)    # (Wickham et al., 2018)
library(cowplot)  # (Wilke, 2018)
library(viridis)  # (Garnier, 2018)
library(scales)

data_dir <- "/Users/amlalejini/devo_ws/ALife-2020--SignalGP-Genetic-Regulation/experiments/alife-2020/data/balanced-reg-mult/reg-graph-comps/dir-sig/"
dump_dir <- "/Users/amlalejini/devo_ws/ALife-2020--SignalGP-Genetic-Regulation/experiments/alife-2020/analysis/"
trace_img_dump_dir <- paste(dump_dir, "imgs/reg-network-comps/dir-sig-traces/", sep="")
trace_ids <- 4001:4100
generation <- 10000


max_fit_data_file <- paste(data_dir, "max_fit_orgs.csv", sep="")
max_fit_data <- read.csv(max_fit_data_file, na.strings="NONE")
max_fit_data <- subset(max_fit_data, select = -c(program))

# Define function to summarize regulation/memory configurations.
get_con <- function(reg, mem) {
  if (reg == "0" && mem == "0") {
    return("none")
  } else if (reg == "0" && mem=="1") {
    return("memory")
  } else if (reg=="1" && mem=="0") {
    return("regulation")
  } else if (reg=="1" && mem=="1") {
    return("both")
  } else {
    return("UNKNOWN")
  }
}
# Specify experimental condition for each datum.
max_fit_data$condition <- mapply(get_con, 
                                 max_fit_data$USE_FUNC_REGULATION, 
                                 max_fit_data$USE_GLOBAL_MEMORY)
max_fit_data$condition <- factor(max_fit_data$condition, 
                                 levels=c("regulation", "memory", "none", "both"))


for (trace_id in trace_ids) {
  org_info <- filter(max_fit_data, SEED==trace_id)
  num_env_states <- org_info$NUM_ENV_STATES
  num_env_updates <- org_info$NUM_ENV_UPDATES
  score <- org_info$aggregate_score
  condition <- org_info$condition
  is_sol <- org_info$solution
  
  if (is_sol == "0") {
    next
  }
  
  trace_file <- paste(data_dir, "traces/",
                      "trace_update-", generation, 
                      "_run-id-", trace_id, ".csv", sep="")
  trace_data <- read.csv(trace_file, na.strings="NONE")
  trace_data$left_similarity_score <- 1 - trace_data$X0_match_score
  trace_data$right_similarity_score <- 1 - trace_data$X1_match_score
  trace_data$triggered <- (trace_data$is_match_cur_dir=="1") & (trace_data$cpu_step == "0")
  trace_data$is_running <- trace_data$is_running > 0 | 
    trace_data$triggered | 
    trace_data$is_cur_responding_module == "1"
  
  # Visualize each environment sequence
  test_ids <- levels(factor(trace_data$test_id))
  for (test_num in test_ids) {
    # test_num <- 0
    test_data <- filter(trace_data, test_id==test_num)
    env_id <- test_num
    env_seq <- test_data$env_seq
    
    # correct response ids  
    response_time_steps <- levels(factor(filter(test_data, is_cur_responding_module=="1")$time_step))
    responses_by_env_update <- list()
    for (t in response_time_steps) {
      env_update <- levels(factor(filter(test_data, time_step==t)$env_update))
      if (env_update %in% names(responses_by_env_update)) {
        if (as.integer(t) > as.integer(responses_by_env_update[env_update])) {
          responses_by_env_update[env_update] = t
        } 
      } else {
        responses_by_env_update[env_update] = t
      }
    }
    # Build a list of modules that were triggered
    triggered_ids <- levels(factor(filter(test_data, triggered==TRUE)$module_id))
    response_ids <- levels(factor(filter(test_data, is_cur_responding_module=="1")$module_id))
    
    test_data$is_ever_active <- test_data$is_ever_active=="1" | 
      test_data$is_running | 
      test_data$module_id %in% triggered_ids | 
      test_data$module_id %in% response_ids
    
    test_data$is_cur_responding_module <- test_data$is_cur_responding_module=="1" &
      test_data$time_step %in% responses_by_env_update
    
    categorize_reg_state <- function(reg_state) {
      if (reg_state == 0) {
        return("neutral")
      } else if (reg_state < 0) {
        return("promoted")
      } else if (reg_state > 0) {
        return("repressed")
      } else {
        return("unknown")
      }
    }
    test_data$regulator_state_simplified <- mapply(categorize_reg_state,
                                                   test_data$regulator_state)
    
    
    # Extract only rows that correspond with modules that were active during evaluation.
    test_data <- filter(test_data, is_ever_active==TRUE)
    # Do some work to have module ids appear in a nice order along axis.
    active_module_ids <- levels(factor(test_data$module_id))
    active_module_ids <- as.integer(active_module_ids)
    module_id_map <- as.data.frame(active_module_ids)
    module_id_map$order <- order(module_id_map$active_module_ids) - 1
    get_module_x_pos <- function(module_id) {
      return(filter(module_id_map, active_module_ids==module_id)$order)
    }
    test_data$mod_id_x_pos <- mapply(get_module_x_pos, test_data$module_id)
    
    plot_title <- paste("Condition: ", condition, "; # Env states: ", num_env_states, ";\nScore: ", score, "; Solution? ", is_sol, "; Test ID: ", env_id, sep="")
    
    out_name <- paste(trace_img_dump_dir, "trace-id-", trace_id, "-test-id-", test_num, "-regulator-state.pdf", sep="")
    ggplot(test_data, 
           aes(x=mod_id_x_pos, 
               y=time_step, 
               fill=regulator_state_simplified)) +
      scale_fill_viridis(name="State:",
                         limits=c("promoted", "neutral", "repressed"),
                         labels=c("Promoted", "Neutral", "Repressed"),
                         discrete=TRUE, 
                         direction=-1) +
      scale_x_discrete(name="Function ID",
                       limits=seq(0, length(active_module_ids)-1, 1),
                       labels=active_module_ids) +
      ylab("Time Step") +
      # Background
      geom_tile(color="white", 
                size=0.2, 
                width=1, 
                height=1) +
      # Module is-running highlights
      geom_tile(data=filter(test_data, is_running==TRUE), 
                color="black", 
                width=1, 
                height=1, 
                size=0.8) +
      # Environment delimiters
      geom_hline(yintercept=filter(test_data, cpu_step==0)$time_step - 0.5, 
                 size=1) +
      # Draw points on triggered modules
      geom_point(data=filter(test_data, triggered==TRUE),
                 shape=8, colour="black", fill="white", stroke=0.5, size=1.5,
                 position=position_nudge(x = 0, y = 0.01)) +
      geom_point(data=filter(test_data, is_cur_responding_module==TRUE),
                 shape=21, colour="black", fill="white", stroke=0.5, size=1.5,
                 position=position_nudge(x = 0, y = 0.01)) +
      theme(legend.position = "top",
            legend.text = element_text(size=10),
            axis.text.y = element_text(size=10),
            axis.text.x = element_text(size=10)) +
      ggtitle(plot_title) +
      ggsave(out_name, height=10, width=8)    
    
    
    out_name <- paste(trace_img_dump_dir, "trace-id-", trace_id, "-test-id-", test_num, "-left-similarity-score.pdf", sep="")
    ggplot(test_data, aes(x=mod_id_x_pos, y=time_step, fill=left_similarity_score)) +
      scale_fill_viridis(option="plasma",
                         name="Score:  ",
                         limits=c(0, 1.0),
                         breaks=c(0, 0.25, 0.50, 0.75, 1.0),
                         labels=c("0%", "25%", "50%", "75%", "100%")) +
      scale_x_discrete(name="Function ID",
                       limits=seq(0, length(active_module_ids)-1, 1),
                       labels=active_module_ids) +
      # Background
      geom_tile(color="white", 
                size=0.2, 
                width=1, 
                height=1) +
      # Module is-running highlights
      geom_tile(data=filter(test_data, is_running==TRUE | triggered==TRUE), 
                color="black", 
                width=1, 
                height=1, 
                size=0.8) +
      # Environment delimiters
      geom_hline(yintercept=filter(test_data, cpu_step==0)$time_step-0.5, size=1) +
      # Draw points on triggered modules
      geom_point(data=filter(test_data, triggered==TRUE),
                 shape=8, colour="black", fill="white", stroke=0.5, size=1.5,
                 position=position_nudge(x = 0, y = 0.01)) +
      geom_point(data=filter(test_data, is_cur_responding_module==TRUE),
                 shape=21, colour="black", fill="white", stroke=0.5, size=1.5,
                 position=position_nudge(x = 0, y = 0.01)) +
      theme(legend.position = "top",
            legend.text = element_text(size=10),
            axis.text.y = element_text(size=10),
            axis.text.x = element_text(size=10)) +
      guides(fill = guide_colourbar(barwidth = 10, barheight = 0.5)) +
      ggtitle(plot_title) +
      ggsave(out_name, height=10, width=8)
    
    out_name <- paste(trace_img_dump_dir, "trace-id-", trace_id, "-test-id-", test_num, "-right-similarity-score.pdf", sep="")
    ggplot(test_data, aes(x=mod_id_x_pos, y=time_step, fill=right_similarity_score)) +
      scale_fill_viridis(option="plasma",
                         name="Score:  ",
                         limits=c(0, 1.0),
                         breaks=c(0, 0.25, 0.50, 0.75, 1.0),
                         labels=c("0%", "25%", "50%", "75%", "100%")) +
      scale_x_discrete(name="Function ID",
                       limits=seq(0, length(active_module_ids)-1, 1),
                       labels=active_module_ids) +
      # Background
      geom_tile(color="white", 
                size=0.2, 
                width=1, 
                height=1) +
      # Module is-running highlights
      geom_tile(data=filter(test_data, is_running==TRUE | triggered==TRUE), 
                color="black", 
                width=1, 
                height=1, 
                size=0.8) +
      # Environment delimiters
      geom_hline(yintercept=filter(test_data, cpu_step==0)$time_step-0.5, size=1) +
      # Draw points on triggered modules
      geom_point(data=filter(test_data, triggered==TRUE),
                 shape=8, colour="black", fill="white", stroke=0.5, size=1.5,
                 position=position_nudge(x = 0, y = 0.01)) +
      geom_point(data=filter(test_data, is_cur_responding_module==TRUE),
                 shape=21, colour="black", fill="white", stroke=0.5, size=1.5,
                 position=position_nudge(x = 0, y = 0.01)) +
      theme(legend.position = "top",
            legend.text = element_text(size=10),
            axis.text.y = element_text(size=10),
            axis.text.x = element_text(size=10)) +
      guides(fill = guide_colourbar(barwidth = 10, barheight = 0.5)) +
      ggtitle(plot_title) +
      ggsave(out_name, height=10, width=8)
  }
}

